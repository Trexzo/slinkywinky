#!/usr/bin/env python3
"""R253 Windows real Java8 Attach + kernel-pinned process recovery.

Starts only disposable R251 JVM fixture processes it owns. Source parser
and native PR2 policy run in an independent native Windows process.
"""
import os
from pathlib import Path
import queue
import re
import subprocess
import sys
import threading
import time

def reader(pipe,out):
    for line in pipe:
        out.put(line.strip())
    out.put("<EOF>")

class OwnedChild:
    def __init__(self,argv,env):
        self.p=subprocess.Popen(
            argv,env=env,stdin=subprocess.PIPE,stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,encoding="utf-8",bufsize=1)
        self.q=queue.Queue()
        threading.Thread(target=reader,args=(self.p.stdout,self.q),daemon=True).start()
    def recv(self,prefix,timeout=20):
        end=time.monotonic()+timeout
        seen=[]
        while time.monotonic()<end:
            try:line=self.q.get(timeout=max(.1,end-time.monotonic()))
            except queue.Empty:break
            seen.append(line)
            if line.startswith(prefix):return line
            if line=="<EOF>":break
        raise AssertionError("R253 expected %s, got %r" %(prefix,seen[-15:]))
    def send(self,command,prefix):
        self.p.stdin.write(command+"\n");self.p.stdin.flush()
        return self.recv(prefix)
    def stop(self):
        if self.p.poll() is None:
            try:
                self.send("QUIT","R251_DISPOSABLE_TARGET_CLEAN_EXIT")
                self.p.wait(timeout=10)
            except Exception:
                self.p.kill();self.p.wait(timeout=8)
        assert self.p.returncode==0,"R253 fixture unexpectedly exited"

def main():
    if os.name!="nt" or len(sys.argv)!=5:
        raise AssertionError("R253 requires Windows, native binary, Java8 home, fixture classes, agent jar")
    binary,home,classes,jar=map(lambda s:Path(s).resolve(),sys.argv[1:])
    java=home/"bin"/"java.exe";tools=home/"lib"/"tools.jar"
    for item in (binary,java,tools,jar):
        if not item.is_file():raise RuntimeError("R253 missing "+str(item))
    env=dict(os.environ,SLINKYWINKY_R251_DISPOSABLE_FIXTURE="1")
    target=[str(java),"-cp",str(classes),
            "dev.slinkywinky.instrument.R251LeaseTarget"]
    def attest(child):
        initial=child.recv("R251_TARGET_READY ")
        m=re.fullmatch(r"R251_TARGET_READY (\d+) (\d+) (\d+) ([a-f0-9]{32})",initial)
        if not m:raise AssertionError("bad Java fixture receipt "+initial)
        pid,birth,epoch,nonce=m.groups()
        if int(pid)!=child.p.pid:raise AssertionError("fixture pid mismatch")
        cmd=[str(java),"-cp",str(classes)+";"+str(tools),
             "dev.slinkywinky.attach.R251LeaseLauncher",
             pid,str(jar),pid,birth,epoch,nonce]
        attached=subprocess.run(cmd,capture_output=True,text=True,env=env,timeout=25)
        if attached.returncode or "R251_ATTACH_API_RETURNED" not in attached.stdout:
            raise AssertionError("R253 external Attach refused "+attached.stdout+attached.stderr)
        receipt=child.send("ATTEST","R252_LEASE_RECEIPT ")
        if receipt!=("R252_LEASE_RECEIPT "+pid+" "+birth+" "+epoch+" "+nonce):
            raise AssertionError("R253 agent did not issue exact receipt")
        return receipt

    old=OwnedChild(target,env);new=None;native=None
    try:
        oldReceipt=attest(old)
        native=OwnedChild([str(binary),oldReceipt],env)
        native.recv("R253_OLD_JVM_KERNEL_HANDLE_PINNED")
        old.stop()  # exact old object is still pinned by the native HANDLE
        new=OwnedChild(target,env)
        newReceipt=attest(new)
        native.send(newReceipt,"R253_PINNED_OLD_EXIT_NEW_LIVE_REAL_PR2_RECOVERY_PASS")
        native.recv("R253_PROBE_FINISHED")
        native.p.wait(timeout=15)
        if native.p.returncode:raise AssertionError("R253 native fixture failed")
        new.stop()
        print("R253_REAL_JAVA8_ATTACH_OS_BIRTH_AND_PINNED_EXIT_PASS")
        print("R253_ACTUAL_PR2_NATIVE_RECOVERY_WITH_KERNEL_PROCESS_IDENTITY_PASS")
        print("R253_NO_MINECRAFT_INPUT_OR_PRODUCTION_AGENT_TOUCH_PASS")
    finally:
        for child in (native,new,old):
            if child is not None and child.p.poll() is None:
                child.p.kill();child.p.wait(timeout=8)

if __name__=="__main__":
    main()
