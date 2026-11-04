#!/usr/bin/env python3
"""R81 acceptance checks. Never calls GitHub, Java, JNI, Attach or Minecraft."""
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "original_189_live_render_acceptance",
    HERE / "original_189_live_render_acceptance.py")
harness = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(harness)

P = harness.PREFLIGHT + "\n"
C = (harness.RESTORED + "\n" + harness.CALLBACK +
     " owner=synthetic#1\n" + harness.RELEASED + "\n")
W = (harness.RESTORED + "\n" + harness.CALLBACK +
     " owner=synthetic#1\n" + harness.STABLE + "\n" +
     harness.RELEASED + "\n")


class ReceiptChecks(unittest.TestCase):
    def test_valid_stages(self):
        self.assertEqual(len(harness.validate_receipt("--render-preflight",P)),1)
        self.assertEqual(len(harness.validate_receipt("--render-callback=8",C)),3)
        self.assertEqual(len(harness.validate_receipt("--render-context=8",W)),4)

    def test_exact_stdout_receipt_lines_resist_prefix_and_suffix_collisions(self):
        wrong = [
            ("--render-preflight",P.replace("PREFLIGHT_PASS","PREFLIGHT_PASS_FAKE")),
            ("--render-preflight","[INFO] "+P),
            ("--render-preflight","fake"+P),
            ("--render-callback=8",C.replace("accepted=8","accepted=80")),
            ("--render-callback=8",C.replace("accepted=8","accepted=8x")),
            ("--render-callback=8",C.replace("owner=synthetic#1","owner=synthetic")),
            ("--render-callback=8",C.replace("owner=synthetic#1","owner=synthetic#0")),
            ("--render-callback=8",C.replace(
                harness.RESTORED,harness.RESTORED+"_FAKE")),
            ("--render-callback=8",C.replace(
                harness.RELEASED,harness.RELEASED+"_FAKE")),
            ("--render-context=8",W.replace("stable=true","stable=trueee")),
            ("--render-context=8",W.replace("current=8","current=80")),
            ("--render-context=8",W.replace("changed=0","changed=00")),
        ]
        for option, log in wrong:
            with self.subTest(option=option, log=log):
                with self.assertRaises(harness.ReceiptError):
                    harness.validate_receipt(option,log)

    def test_conflicting_fake_receipt_even_after_true_receipt_is_denied(self):
        for option, good, counterfeit in [
            ("--render-preflight",P,"noise "+harness.PREFLIGHT+"\n"),
            ("--render-callback=8",C,harness.CALLBACK+"0 owner=bad#5\n"),
            ("--render-context=8",W,harness.STABLE+"_TRAILING\n")
        ]:
            with self.subTest(option=option):
                with self.assertRaises(harness.ReceiptError):
                    harness.validate_receipt(option,good+counterfeit)

    def test_windows_crlf_and_owner_with_spaces_are_accepted(self):
        callback=C.replace("synthetic#1","Client thread#23")
        for option,log in [
            ("--render-preflight",P),("--render-callback=8",callback),
            ("--render-context=8",W.replace("synthetic#1","Client thread#23"))
        ]:
            with self.subTest(option=option):
                self.assertEqual(
                    tuple(harness.STAGES["context"][
                        ("--render-preflight","--render-callback=8","--render-context=8").index(option)
                    ][1]),
                    harness.validate_receipt(option,log.replace("\n","\r\n")))

    def test_old_or_partial_receipts_never_pass(self):
        for option, text in [
            ("--render-preflight",""),
            ("--render-callback=8",harness.RESTORED+"\n"),
            ("--render-callback=8",harness.CALLBACK+"\n"),
            ("--render-context=8",C),
            ("--render-context=8",harness.STABLE),
            ("--render-context=8",harness.RESTORED+"\n"+harness.STABLE),
            ("--render-callback=8",harness.RESTORED+"\n"+harness.CALLBACK+"\n"),
            ("--render-context=8",harness.RESTORED+"\n"+harness.CALLBACK+"\n"+harness.STABLE+"\n"),
            ("--render-callback=8",harness.RELEASED+"\n"+harness.RESTORED+"\n"+harness.CALLBACK+"\n"),
            ("--render-context=8",harness.RELEASED+"\n"+harness.RESTORED+"\n"+harness.CALLBACK+"\n"+harness.STABLE+"\n"),
            ("--render-preflight",P+P),
            ("--render-callback=8",C+C),
            ("--render-context=8",W+W)
        ]:
            with self.subTest(option=option,text=text):
                with self.assertRaises(harness.ReceiptError):
                    harness.validate_receipt(option,text)

    def test_overlapping_diagnostic_stages_do_not_count_as_fresh_proof(self):
        for option, text in [
            ("--render-preflight",P+C),
            ("--render-callback=8",P+C),
            ("--render-context=8",P+W)
        ]:
            with self.subTest(option=option):
                with self.assertRaises(harness.ReceiptError):
                    harness.validate_receipt(option,text)

    def test_negative_target_markers_win_over_positive(self):
        for bad in harness.NEGATIVE:
            with self.subTest(bad=bad):
                with self.assertRaises(harness.ReceiptError):
                    harness.validate_receipt("--render-context=8",W+bad+"\n")

    def test_external_attach_receipt_cannot_substitute_target_log(self):
        with self.assertRaises(harness.ReceiptError):
            harness.validate_receipt("--render-callback=8",
                                     C+"ATTACH_RESULT=SUCCESS pid=123\n")

    def test_only_one_original_vanilla_main(self):
        self.assertEqual(
            harness.check_target_listing(
                "123 net.minecraft.client.main.Main [minecraft-candidate]\n",123),
            "net.minecraft.client.main.Main [minecraft-candidate]")
        for listing,pid in [
            ("123 net.minecraft.client.main.Main\n",124),
            ("",123),
            ("123 net.minecraft.client.main.Main\n456 net.minecraft.client.main.Main\n",123),
            ("123 GradleStart\n",123),
            ("123 net.minecraft.client.main.MainMalicious\n",123),
            ("123 net.minecraft.client.main.Main\n456 GradleStart\n",123),
            ("123 net.minecraft.client.main.Main\n456 net.minecraft.launchwrapper.Launch\n",123)
        ]:
            with self.subTest(listing=listing):
                with self.assertRaises(harness.ReceiptError):
                    harness.check_target_listing(listing,pid)

    def test_only_appended_bytes_count(self):
        with tempfile.TemporaryDirectory() as folder:
            log=Path(folder)/"stdout.log"
            log.write_bytes((P+C).encode())
            baseline=log.stat().st_size
            with log.open("ab") as stream:
                stream.write(W.encode())
            self.assertEqual(harness.read_appended(log,baseline),W)
            self.assertEqual(len(harness.validate_receipt(
                "--render-context=8",harness.read_appended(log,baseline))),4)
            log.write_text("rotated",encoding="utf-8")
            with self.assertRaises(harness.ReceiptError):
                harness.read_appended(log,baseline)

    def test_replaced_log_with_larger_stale_receipts_is_denied(self):
        with tempfile.TemporaryDirectory() as folder:
            log=Path(folder)/"stdout.log"
            replacement=Path(folder)/"replacement.log"
            log.write_text(P,encoding="utf-8")
            original=log.stat()
            replacement.write_text(P+C+W,encoding="utf-8")
            replacement.replace(log)
            with self.assertRaises(harness.ReceiptError):
                harness.read_appended(log,original.st_size,original)

    @unittest.skipUnless(os.name=="nt", "R82 simulated --execute requires Windows")
    def test_execute_mocked_three_stage_acceptance_emits_sanitized_report(self):
        # Mock ONLY the external loader process; do not start a JVM or game.
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            loader=root/"slinkywinky_loader.exe"
            java=root/"jdk8"/"bin"/"java.exe"
            tools=root/"jdk8"/"lib"/"tools.jar"
            log=root/"target-stdout.log"
            report=root/"r82-evidence.json"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log):
                file.touch()
            attached=[]
            def fake(cmd,**kwargs):
                self.assertNotIn("SLINKYWINKY_ATTACH_TEST",kwargs["env"])
                self.assertNotIn("SLINKYWINKY_EXPERIMENTAL_HOOK",kwargs["env"])
                self.assertEqual(cmd[0],str(loader))
                if cmd[1]=="list":
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"123 net.minecraft.client.main.Main\n","")
                self.assertEqual(cmd[1],"attach")
                self.assertEqual(cmd[2],"123")
                option=cmd[-1]
                attached.append(option)
                stage={"--render-preflight":P,
                       "--render-callback=8":C,
                       "--render-context=8":W}[option]
                with log.open("a",encoding="utf-8") as output:
                    output.write(stage)
                return harness.subprocess.CompletedProcess(
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode=fixture\n","")
            with patch.object(harness.subprocess,"run",side_effect=fake):
                result=harness.run([
                    "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                    "--pid","123","--target-stdout-log",str(log),
                    "--mode","context","--execute","--report",str(report)])
            self.assertEqual(result,0)
            self.assertEqual(attached,
                             ["--render-preflight","--render-callback=8",
                              "--render-context=8"])
            data=json.loads(report.read_text(encoding="utf-8"))
            self.assertEqual(len(data["stages"]),3)
            self.assertFalse(data["visual_game_acceptance"])
            self.assertFalse(data["in_game_menu_verified"])
            self.assertNotIn("pointer",report.read_text(encoding="utf-8"))

    @unittest.skipUnless(os.name=="nt", "R82 simulated failure requires Windows")
    def test_execute_mocked_restoration_failure_never_starts_wgl_stage(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            loader=root/"slinkywinky_loader.exe"
            java=root/"jdk8"/"bin"/"java.exe"
            tools=root/"jdk8"/"lib"/"tools.jar"
            log=root/"target-stdout.log"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log):
                file.touch()
            attached=[]
            def fake(cmd,**kwargs):
                if cmd[1]=="list":
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"123 net.minecraft.client.main.Main\n","")
                option=cmd[-1]
                attached.append(option)
                with log.open("a",encoding="utf-8") as output:
                    output.write(P if option=="--render-preflight" else
                                 "R75_RESTORE_UNCERTAIN_NO_REATTACH\n")
                return harness.subprocess.CompletedProcess(
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123\n","")
            with patch.object(harness.subprocess,"run",side_effect=fake):
                with self.assertRaises(harness.ReceiptError):
                    harness.run([
                        "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                        "--pid","123","--target-stdout-log",str(log),
                        "--mode","context","--execute"])
            self.assertEqual(attached,["--render-preflight",
                                       "--render-callback=8"])

    def test_plan_never_launches_attach_or_reads_process(self):
        with patch.object(harness.subprocess,"run",
                          side_effect=AssertionError("plan started a subprocess")):
            result=harness.run([
                "--loader","Z:/nonexistent/slinkywinky_loader.exe",
                "--jdk8","Z:/nonexistent/java8","--pid","500",
                "--target-stdout-log","Z:/nonexistent/game.log",
                "--mode","context"])
        self.assertEqual(result,0)

    def test_execute_rejects_unsupported_environment(self):
        if harness.os.name=="nt":
            self.skipTest("Windows runner checks tested separately")
        with patch.object(harness.subprocess,"run",
                          side_effect=AssertionError("executed on non-Windows")):
            with self.assertRaises(harness.ReceiptError):
                harness.run([
                    "--loader","Z:/untrusted.exe","--jdk8","Z:/java8",
                    "--pid","500","--target-stdout-log","Z:/game.log",
                    "--execute"])

    def test_invalid_pid_and_timeout_are_refused(self):
        common=["--loader","fake","--jdk8","fake","--target-stdout-log","fake"]
        for pid in ("0","-1","4294967296"):
            with self.subTest(pid=pid):
                with self.assertRaises(harness.ReceiptError):
                    harness.run(common+["--pid",pid])
        for timeout in ("0","14","61"):
            with self.subTest(timeout=timeout):
                with self.assertRaises(harness.ReceiptError):
                    harness.run(common+["--pid","10","--timeout-seconds",timeout])


if __name__=="__main__":
    unittest.main()
