#!/usr/bin/env python3
"""R81 acceptance checks. Never calls GitHub, Java, JNI, Attach or Minecraft."""
import importlib.util
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
C = harness.RESTORED + "\n" + harness.CALLBACK + " owner=synthetic#1\n"
W = C + harness.STABLE + "\n"


class ReceiptChecks(unittest.TestCase):
    def test_valid_stages(self):
        self.assertEqual(len(harness.validate_receipt("--render-preflight",P)),1)
        self.assertEqual(len(harness.validate_receipt("--render-callback=8",C)),2)
        self.assertEqual(len(harness.validate_receipt("--render-context=8",W)),3)

    def test_old_or_partial_receipts_never_pass(self):
        for option, text in [
            ("--render-preflight",""),
            ("--render-callback=8",harness.RESTORED+"\n"),
            ("--render-callback=8",harness.CALLBACK+"\n"),
            ("--render-context=8",C),
            ("--render-context=8",harness.STABLE),
            ("--render-context=8",harness.RESTORED+"\n"+harness.STABLE),
            ("--render-preflight",P+P),
            ("--render-callback=8",C+C),
            ("--render-context=8",W+W)
        ]:
            with self.subTest(option=option,text=text):
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
                "--render-context=8",harness.read_appended(log,baseline))),3)
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
