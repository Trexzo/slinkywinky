#!/usr/bin/env python3
"""R251: real external Attach API against owned disposable Java 8 JVMs.

Not a Minecraft attach test. Never scans or selects an arbitrary JVM. Proofs
are fixture-issued, not a trusted OS or production game-session attestation.
"""
import os
from pathlib import Path
import queue
import re
import subprocess
import sys
import threading
import time
import zipfile


class Child:
    def __init__(self, java, classes, environment):
        self.proc = subprocess.Popen(
            [str(java), '-cp', str(classes),
             'dev.slinkywinky.instrument.R251LeaseTarget'],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, encoding='utf-8', bufsize=1,
            env=environment)
        self.lines = queue.Queue()
        self.thread = threading.Thread(target=self._read, daemon=True)
        self.thread.start()

    def _read(self):
        for line in self.proc.stdout:
            self.lines.put(line.strip())
        self.lines.put('<EOF>')

    def get(self, prefix, timeout=18):
        until = time.monotonic() + timeout
        seen = []
        while time.monotonic() < until:
            try:
                line = self.lines.get(timeout=max(0.1, until-time.monotonic()))
            except queue.Empty:
                break
            seen.append(line)
            if line.startswith(prefix):
                return line
            if line == '<EOF>':
                break
        raise AssertionError('R251 missing %s; observed=%r' % (prefix, seen[-30:]))

    def send(self, action, marker):
        self.proc.stdin.write(action + '\n')
        self.proc.stdin.flush()
        return self.get(marker)

    def close(self):
        if self.proc.poll() is None:
            try:
                self.send('QUIT', 'R251_DISPOSABLE_TARGET_CLEAN_EXIT')
                self.proc.wait(timeout=8)
            except (OSError, AssertionError, subprocess.TimeoutExpired):
                self.proc.terminate()
                try:
                    self.proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    self.proc.kill()
                    self.proc.wait(timeout=5)
        assert self.proc.returncode == 0, 'R251 disposable JVM exited abnormally'


def parse_proof(line):
    match = re.fullmatch(
        r'R251_TARGET_(?:READY|NEW_EPOCH) (\d+) (\d+) (\d+) ([a-f0-9]{32})', line)
    if not match:
        raise AssertionError('Invalid R251 fixture proof: ' + line)
    return (int(match[1]), int(match[2]), int(match[3]), match[4])


def check_status(child, active, poisoned, generation):
    result = child.send('STATUS', 'R251_STATUS ')
    expected = 'R251_STATUS active=%s poisoned=%s lastGeneration=%d' % (
        str(active).lower(), str(poisoned).lower(), generation)
    assert result == expected, (result, expected)


def main():
    if len(sys.argv) != 4:
        raise ValueError('Expected built loader, JDK, and native R252 probe')
    probe = Path(sys.argv[3]).resolve()
    if not probe.is_file():
        raise ValueError('R252 native C++ ABI probe not built')
    loader = Path(sys.argv[1]).resolve()
    home = Path(sys.argv[2]).resolve()
    staged = loader.parent
    classes = staged / 'attach-test-classes'
    jar = staged / 'slinkywinky_r251_test_agent.jar'
    production = staged / 'slinkywinky_instrument.jar'
    java = home / 'bin' / ('java.exe' if os.name == 'nt' else 'java')
    tools = home / 'lib' / 'tools.jar'
    for file in (jar, production, java):
        if not file.is_file():
            raise RuntimeError('R251 absent required dependency: ' + str(file))
    if os.name == 'nt' and not tools.is_file():
        raise RuntimeError('R251 requires real Java8 tools.jar')
    with zipfile.ZipFile(production) as z:
        if any('R251Lease' in name for name in z.namelist()):
            raise AssertionError('R251 fixture was packaged into production agent')
    with zipfile.ZipFile(jar) as z:
        if 'dev/slinkywinky/instrument/R251LeaseAgent.class' not in z.namelist():
            raise AssertionError('R251 isolated agent absent')

    env = os.environ.copy()
    env['SLINKYWINKY_R251_DISPOSABLE_FIXTURE'] = '1'

    def verify_native_receipt(receipt, proof):
        pid, birth, generation, challenge = proof
        invocation = [str(probe), str(pid), str(birth),
                      str(generation), challenge, receipt]
        result = subprocess.run(
            invocation, capture_output=True, encoding='utf-8', timeout=10)
        if result.returncode or (
                'R252_JAVA8_ACTUAL_R251_RECEIPT_NATIVE_ABI_PASS'
                not in result.stdout):
            raise AssertionError('R252 C++ verification of actual Java'
                                 ' lease failed: ' + result.stdout +
                                 result.stderr)
        # Tampering with the source-approved receipt must NEVER be
        # silently accepted by the C++ bridge parser.
        for changed in (
                (pid + 1, birth, generation, challenge),
                (pid, birth + 1, generation, challenge),
                (pid, birth, generation + 1, challenge),
                (pid, birth, generation, 'f' * 32)):
            invalid = [str(probe), str(changed[0]), str(changed[1]),
                       str(changed[2]), changed[3], receipt]
            denied = subprocess.run(
                invalid, capture_output=True, encoding='utf-8', timeout=10)
            if denied.returncode == 0:
                raise AssertionError('R252 native parser accepted '
                                     'mismatched JVM evidence')
        print('R252_REAL_JAVA8_RECEIPT_CROSS_LANGUAGE_VALIDATED')

    def attempt(child, proof, claimed_pid=None):
        pid, birth, generation, challenge = proof
        args = [str(java), '-cp', str(classes) + os.pathsep + str(tools),
                'dev.slinkywinky.attach.R251LeaseLauncher',
                str(child.proc.pid), str(jar),
                str(pid if claimed_pid is None else claimed_pid),
                str(birth), str(generation), challenge]
        out = subprocess.run(args, capture_output=True, encoding='utf-8',
                             env=env, timeout=25)
        if out.returncode != 0 or 'R251_ATTACH_API_' not in out.stdout:
            raise AssertionError('R251 external Attach invocation failed: ' +
                                 out.stdout + '\n' + out.stderr)
        return out.stdout

    first = Child(java, classes, env)
    second = None
    try:
        one = parse_proof(first.get('R251_TARGET_READY '))
        assert one[0] == first.proc.pid
        attempt(first, one, claimed_pid=one[0]+1)
        check_status(first, False, False, 0)
        attempt(first, (one[0], one[1]+1, one[2], one[3]))
        check_status(first, False, False, 0)
        attempt(first, (one[0], one[1], one[2], '0'*32))
        check_status(first, False, False, 0)
        attempt(first, one)
        check_status(first, True, False, 1)
        verify_native_receipt(
            first.send('ATTEST', 'R252_LEASE_RECEIPT '), one)
        attempt(first, one)  # competing controller must not steal the lease
        check_status(first, True, False, 1)
        first.send('FOREIGN_RELEASE', 'R251_FOREIGN_RELEASE_DENIED')
        check_status(first, True, False, 1)
        next_proof = parse_proof(first.send('ROTATE', 'R251_TARGET_NEW_EPOCH '))
        assert next_proof[0:2] == one[0:2]
        assert next_proof[2] == 2 and next_proof[3] != one[3]
        check_status(first, False, False, 1)
        first.send('ATTEST', 'R252_LEASE_REFUSED')
        attempt(first, one)  # stale generation and old challenge
        check_status(first, False, False, 1)
        attempt(first, (one[0], one[1]+1, 2, next_proof[3]))
        check_status(first, False, False, 1)
        attempt(first, next_proof)
        check_status(first, True, False, 2)
        verify_native_receipt(
            first.send('ATTEST', 'R252_LEASE_RECEIPT '), next_proof)
        first.send('POISON', 'R251_UNCERTAIN_OWNER_LOCKED')
        check_status(first, True, True, 2)
        first.send('ATTEST', 'R252_LEASE_REFUSED')
        attempt(first, next_proof)
        check_status(first, True, True, 2)
        first.close()
        print('R251_DISPOSABLE_REAL_ATTACH_PID_BIRTH_CHALLENGE_PASS')
        print('R251_DUPLICATE_REPLAY_FOREIGN_RELEASE_UNCERTAIN_LOCK_PASS')

        second = Child(java, classes, env)
        new = parse_proof(second.get('R251_TARGET_READY '))
        assert (new[0], new[1]) != (one[0], one[1]), 'R251 reused process incarnation'
        # Replays against a *different* JVM fail, regardless of PID reuse.
        attempt(second, one, claimed_pid=one[0])
        check_status(second, False, False, 0)
        attempt(second, new)
        check_status(second, True, False, 1)
        verify_native_receipt(
            second.send('ATTEST', 'R252_LEASE_RECEIPT '), new)
        second.send('ROTATE', 'R251_TARGET_NEW_EPOCH ')
        check_status(second, False, False, 1)
        second.close()
        print('R251_NEW_JVM_DISTINCT_INCARNATION_ADMITTED_PASS')
        print('R251_NO_MINECRAFT_PROCESS_OR_INPUT_WEAVE_PASS')
    finally:
        for item in (second, first):
            if item is not None and item.proc.poll() is None:
                try:
                    item.close()
                except Exception:
                    item.proc.kill()
                    item.proc.wait(timeout=5)


if __name__ == '__main__':
    main()
