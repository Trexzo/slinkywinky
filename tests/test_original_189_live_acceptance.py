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

P = harness.PREFLIGHT + "\n" + harness.PREFLIGHT_RELEASED + "\n"
C = (harness.RESTORED + "\n" + harness.CALLBACK +
     " owner=synthetic#1\n" + harness.RELEASED + "\n")
W = (harness.RESTORED + "\n" + harness.CALLBACK +
     " owner=synthetic#1\n" + harness.STABLE + "\n" +
     harness.RELEASED + "\n")


O = (harness.RESTORED + "\n" + harness.CALLBACK +
     " owner=synthetic#1\n" + harness.STABLE + "\n" +
     harness.OVERLAY + "\n" + harness.RELEASED + "\n")

M = (harness.RESTORED + "\n" + harness.R92_CALLBACK +
     " owner=synthetic#1\n" + harness.R92_STABLE + "\n" +
     harness.R92_OVERLAY + "\n" + harness.RELEASED + "\n")


V = (harness.RESTORED + "\n" + harness.R93_CALLBACK +
     " owner=synthetic#1\n" + harness.R93_STABLE + "\n" +
     harness.R93_VISIBILITY + "\n" + harness.RELEASED + "\n")


U = (harness.RESTORED + "\n"
     + harness.R94_CALLBACK + " accepted=42 owner=synthetic#1\n"
     + harness.R94_STABLE + " attempts=42 current=42 absent=0 invalid=0 "
       "changed=0 stable=true shown=12 hidden=22 toggles=3 stop=operator\n"
     + harness.R94_VISIBILITY + " frames=42 toggles=3 shown=12 hidden=22 "
       "stop=operator\n" + harness.RELEASED + "\n")


K = (U.replace("toggles=3","toggles=2")
     .replace(harness.R94_VISIBILITY,
              "R95_INPUT_SOURCE_AUDIT keys=2 events=0\\n" + harness.R94_VISIBILITY))


class ReceiptChecks(unittest.TestCase):
    def test_r95_insert_source_receipt_and_sandbox_binding_collision(self):
        self.assertEqual(len(harness.validate_receipt("--render-visibility=manual",K)),5)
        for modified in (
            K.replace("keys=2","keys=1"),
            K.replace("events=0","events=1"),
            K.replace("R95_INPUT_SOURCE_AUDIT","R95_INPUT_SOURCE_FAKED"),
        ):
            self.assertNotEqual(modified,K)
        from windows_r95_scoped_insert import check_offline_insert_collision
        with tempfile.TemporaryDirectory() as root:
            folder=Path(root)/"game"
            folder.mkdir()
            options=folder/"options.txt"
            options.write_text("key_key.jump:210\\n",encoding="utf-8")
            with self.assertRaises(RuntimeError):
                check_offline_insert_collision(Path(root))
            options.write_text("key_key.jump:57\\n",encoding="utf-8")
            check_offline_insert_collision(Path(root))

    def test_r94_manual_exact_dynamic_counters_and_fake_receipt_refusal(self):
        self.assertEqual(len(harness.validate_receipt("--render-visibility=manual",U)),5)
        harness.validate_loader_receipt(
            "--render-visibility=manual",123,
            "ATTACH_RESULT=SUCCESS pid=123 mode=render-manual-r94\n")
        for incorrect in (
            U.replace(" accepted=42 "," accepted=43 "),
            U.replace(" attempts=42 "," attempts=43 "),
            U.replace(" current=42 "," current=43 "),
            U.replace(" shown=12 "," shown=13 "),
            U.replace(" hidden=22 "," hidden=23 "),
            U.replace(" toggles=3 "," toggles=1 "),
            U.replace(" stop=operator"," stop=limit"),
            U.replace(harness.R94_VISIBILITY, "R94_OPERATOR_VISIBILITY_UNVERIFIED"),
            U.replace(harness.R94_VISIBILITY,harness.R94_VISIBILITY+"_FAKE"),
            U.replace(harness.R94_STABLE+" attempts=42",
                      harness.R94_STABLE+" attempts=2048"),
            U+U,
            U+"R94_WGL_CONTEXT_UNVERIFIED\n",
        ):
            with self.subTest(bad=incorrect),self.assertRaises(harness.ReceiptError):
                harness.validate_receipt("--render-visibility=manual",incorrect)
        for mode,log in (
            ("--render-visibility=manual",V),
            ("--render-visibility=72",U),
            ("--render-overlay=72",U),
        ):
            with self.assertRaises(harness.ReceiptError):
                harness.validate_receipt(mode,log)

    def test_r93_show_hide_show_receipts_are_ordered_and_fail_closed(self):
        self.assertEqual(len(harness.validate_receipt("--render-visibility=72",V)),5)
        harness.validate_loader_receipt(
            "--render-visibility=72",123,
            "ATTACH_RESULT=SUCCESS pid=123 mode=render-visibility-r93\n")
        for forged in (
            V.replace(harness.R93_VISIBILITY,""),
            V.replace("shown=48","shown=49"),
            V.replace("hidden=16","hidden=15"),
            V.replace("current=72","current=71"),
            V.replace("accepted=72","accepted=73"),
            V.replace(harness.R93_VISIBILITY,harness.R93_VISIBILITY+"_FAKE"),
            V.replace(harness.R93_STABLE+"\n"+harness.R93_VISIBILITY,
                      harness.R93_VISIBILITY+"\n"+harness.R93_STABLE),
            V.replace(harness.R93_VISIBILITY,
                      harness.R93_VISIBILITY+"\n"+harness.R93_VISIBILITY),
            V+V,
            V+"R93_WGL_CONTEXT_UNVERIFIED\n",
            V+"R93_MENU_VISIBILITY_UNVERIFIED\n",
        ):
            with self.subTest(forged=forged),self.assertRaises(harness.ReceiptError):
                harness.validate_receipt("--render-visibility=72",forged)
        for mode,log in (
            ("--render-visibility=72",M),
            ("--render-visibility=72",O),
            ("--render-overlay=72",V),
            ("--render-overlay=8",V),
        ):
            with self.assertRaises(harness.ReceiptError):
                harness.validate_receipt(mode,log)

    def test_r92_exact_72_callback_64_frame_receipts_and_negative_controls(self):
        self.assertEqual(len(harness.validate_receipt("--render-overlay=72",M)),5)
        harness.validate_loader_receipt(
            "--render-overlay=72",123,
            "ATTACH_RESULT=SUCCESS pid=123 mode=render-overlay-r92\n")
        for invalid in (
            M.replace(harness.R92_OVERLAY,""),
            M.replace("accepted=72","accepted=8"),
            M.replace("frames=64","frames=63"),
            M.replace("current=72","current=71"),
            M.replace("invalid=0","invalid=1"),
            M.replace("stable=true","stable=false"),
            M.replace(harness.R92_OVERLAY,harness.R92_OVERLAY+"_FAKE"),
            M.replace(harness.R92_STABLE+"\n"+harness.R92_OVERLAY,
                      harness.R92_OVERLAY+"\n"+harness.R92_STABLE),
            M.replace(harness.R92_OVERLAY,
                      harness.R92_OVERLAY+"\n"+harness.R92_OVERLAY),
            M + M,
            M + "R92_WGL_CONTEXT_UNVERIFIED\n",
            M + "R92_MULTIFRAME_OVERLAY_UNVERIFIED\n",
        ):
            with self.subTest(invalid=invalid),self.assertRaises(harness.ReceiptError):
                harness.validate_receipt("--render-overlay=72",invalid)
        for option,log in (
            ("--render-overlay=72",O),
            ("--render-overlay=8",M),
            ("--render-context=8",M),
        ):
            with self.assertRaises(harness.ReceiptError):
                harness.validate_receipt(option,log)

    def test_r91_overlay_receipts_are_fresh_strict_and_separate(self):
        self.assertEqual(len(harness.validate_receipt("--render-overlay=8",O)),5)
        for malformed in (
            O.replace(harness.OVERLAY,""),
            O.replace(harness.OVERLAY,harness.OVERLAY+"_FAKE"),
            O.replace(harness.OVERLAY,harness.OVERLAY.replace("frames=1","frames=2")),
            O.replace(harness.STABLE+"\n"+harness.OVERLAY,
                      harness.OVERLAY+"\n"+harness.STABLE),
            O+O,
            O.replace(harness.OVERLAY,harness.OVERLAY+"\n"+
                      harness.OVERLAY),
            O+"R91_RENDER_OVERLAY_UNVERIFIED\n",
        ):
            with self.subTest(log=malformed),self.assertRaises(harness.ReceiptError):
                harness.validate_receipt("--render-overlay=8",malformed)
        with self.assertRaises(harness.ReceiptError):
            harness.validate_receipt("--render-context=8",O)
        with self.assertRaises(harness.ReceiptError):
            harness.validate_receipt("--render-overlay=8",W)
        harness.validate_loader_receipt(
            "--render-overlay=8",123,
            "ATTACH_RESULT=SUCCESS pid=123 mode=render-overlay-r91\n")

    def test_valid_stages(self):
        self.assertEqual(len(harness.validate_receipt("--render-preflight",P)),2)
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
            ("--render-preflight",harness.PREFLIGHT+"\n"),
            ("--render-preflight",harness.PREFLIGHT_RELEASED+"\n"),
            ("--render-preflight",harness.PREFLIGHT_RELEASED+"\n"+harness.PREFLIGHT+"\n"),
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

    def test_loader_receipt_must_match_exact_pid_and_diagnostic_mode(self):
        for option in ("--render-preflight","--render-callback=8",
                       "--render-context=8"):
            valid=("ATTACH_RESULT=SUCCESS pid=123 mode="+
                   harness.LOADER_MODES[option]+"\n")
            harness.validate_loader_receipt(option,123,valid)
            invalid=[
                valid.replace("pid=123 ","pid=1234 "),
                valid.replace("pid=123 ","pid=12 "),
                valid.replace("SUCCESS","FAIL"),
                valid.replace("mode="+harness.LOADER_MODES[option],
                              "mode=some-other-diagnostic"),
                valid+"ATTACH_RESULT=SUCCESS pid=123 mode="+
                    harness.LOADER_MODES[option]+"\n",
                "noise "+valid,
                valid.replace("\n"," trailing text\n"),
                valid.replace("pid=123 ","pid=123x "),
            ]
            for output in invalid:
                with self.subTest(option=option,output=output):
                    with self.assertRaises(harness.ReceiptError):
                        harness.validate_loader_receipt(option,123,output)

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

    def test_target_stdout_evidence_window_is_bounded(self):
        with tempfile.TemporaryDirectory() as folder:
            log=Path(folder)/"target-stdout.log"
            log.write_bytes(b"x"*(harness.MAX_STAGE_STDOUT_BYTES+1))
            with self.assertRaises(harness.ReceiptError):
                harness.read_appended(log,0)

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
            client=root/"fixture-original-client.jar"
            report=root/"r82-evidence.json"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log,client):
                file.touch()
            attached=[]
            def fake(cmd,**kwargs):
                self.assertNotIn("SLINKYWINKY_ATTACH_TEST",kwargs["env"])
                self.assertNotIn("SLINKYWINKY_EXPERIMENTAL_HOOK",kwargs["env"])
                self.assertEqual(cmd[0],str(loader.resolve()))
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
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+
                    harness.LOADER_MODES[option]+"\n","")
            with patch.object(harness,"process_birth_token",return_value=8123456789), \
                 patch.object(harness,"verify_original_client_jar",return_value=(1,2,3,4)), \
                 patch.object(harness.subprocess,"run",side_effect=fake):
                result=harness.run([
                    "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                    "--pid","123","--target-stdout-log",str(log),
                    "--original-client-jar",str(client),
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

    @unittest.skipUnless(os.name=="nt", "R91 synthetic overlay requires Windows")
    def test_r91_four_stage_overlay_attach_gate_and_receipts(self):
        # Fully simulated Attach and JVM stdout: NEVER start Minecraft.
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            loader=root/"slinkywinky_loader.exe"
            java=root/"jdk8"/"bin"/"java.exe"
            tools=root/"jdk8"/"lib"/"tools.jar"
            log=root/"target-stdout.log"
            client=root/"fixture-original-client.jar"
            report=root/"r91-evidence.json"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log,client):
                file.touch()
            attached=[]
            def fake(cmd,**kwargs):
                self.assertNotIn("SLINKYWINKY_ATTACH_TEST",kwargs["env"])
                self.assertNotIn("SLINKYWINKY_EXPERIMENTAL_HOOK",kwargs["env"])
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_OVERLAY"),"1")
                if cmd[1]=="list":
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"123 net.minecraft.client.main.Main\n","")
                self.assertEqual(cmd[1],"attach")
                self.assertEqual(cmd[2],"123")
                option=cmd[-1]
                attached.append(option)
                stage={"--render-preflight":P,
                       "--render-callback=8":C,
                       "--render-context=8":W,
                       "--render-overlay=8":O}[option]
                with log.open("a",encoding="utf-8") as output:
                    output.write(stage)
                return harness.subprocess.CompletedProcess(
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+
                    harness.LOADER_MODES[option]+"\n","")
            with patch.object(harness,"process_birth_token",return_value=81), \
                 patch.object(harness,"verify_original_client_jar",return_value=(1,2,3,4)), \
                 patch.object(harness.subprocess,"run",side_effect=fake):
                result=harness.run([
                    "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                    "--pid","123","--target-stdout-log",str(log),
                    "--original-client-jar",str(client),
                    "--mode","overlay","--execute","--report",str(report)])
            self.assertEqual(result,0)
            self.assertEqual(attached,
                             ["--render-preflight","--render-callback=8",
                              "--render-context=8","--render-overlay=8"])
            data=json.loads(report.read_text(encoding="utf-8"))
            self.assertEqual(len(data["stages"]),4)
            self.assertEqual(data["stages"][-1]["mode"],"--render-overlay=8")
            self.assertFalse(data["visual_game_acceptance"])
            self.assertFalse(data["in_game_menu_verified"])

    @unittest.skipUnless(os.name=="nt", "R92 synthetic multiframe requires Windows")
    def test_r92_five_stage_multiframe_attach_gate_and_receipts(self):
        # Fully simulated Attach and JVM stdout: NEVER start Minecraft.
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            loader=root/"slinkywinky_loader.exe"
            java=root/"jdk8"/"bin"/"java.exe"
            tools=root/"jdk8"/"lib"/"tools.jar"
            log=root/"target-stdout.log"
            client=root/"fixture-original-client.jar"
            report=root/"r92-evidence.json"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log,client):
                file.touch()
            attached=[]
            def fake(cmd,**kwargs):
                self.assertNotIn("SLINKYWINKY_ATTACH_TEST",kwargs["env"])
                self.assertNotIn("SLINKYWINKY_EXPERIMENTAL_HOOK",kwargs["env"])
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_OVERLAY"),"1")
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_MULTIFRAME"),"1")
                if cmd[1]=="list":
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"123 net.minecraft.client.main.Main\n","")
                self.assertEqual(cmd[1],"attach")
                self.assertEqual(cmd[2],"123")
                option=cmd[-1]
                attached.append(option)
                stage={"--render-preflight":P,
                       "--render-callback=8":C,
                       "--render-context=8":W,
                       "--render-overlay=8":O,
                       "--render-overlay=72":M}[option]
                with log.open("a",encoding="utf-8") as output:
                    output.write(stage)
                return harness.subprocess.CompletedProcess(
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+
                    harness.LOADER_MODES[option]+"\n","")
            with patch.object(harness,"process_birth_token",return_value=81), \
                 patch.object(harness,"verify_original_client_jar",return_value=(1,2,3,4)), \
                 patch.object(harness.subprocess,"run",side_effect=fake):
                result=harness.run([
                    "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                    "--pid","123","--target-stdout-log",str(log),
                    "--original-client-jar",str(client),
                    "--mode","multiframe","--execute","--report",str(report)])
            self.assertEqual(result,0)
            self.assertEqual(attached,
                             ["--render-preflight","--render-callback=8",
                              "--render-context=8","--render-overlay=8",
                              "--render-overlay=72"])
            data=json.loads(report.read_text(encoding="utf-8"))
            self.assertEqual(len(data["stages"]),5)
            self.assertEqual(data["stages"][-1]["mode"],"--render-overlay=72")
            self.assertFalse(data["visual_game_acceptance"])
            self.assertFalse(data["in_game_menu_verified"])

    @unittest.skipUnless(os.name=="nt", "R93 synthetic visibility requires Windows")
    def test_r93_six_stage_visibility_attach_and_release(self):
        # Fully simulated Attach and JVM stdout: NEVER start Minecraft.
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            loader=root/"slinkywinky_loader.exe"
            java=root/"jdk8"/"bin"/"java.exe"
            tools=root/"jdk8"/"lib"/"tools.jar"
            log=root/"target-stdout.log"
            client=root/"fixture-original-client.jar"
            report=root/"r93-evidence.json"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log,client):
                file.touch()
            attached=[]
            def fake(cmd,**kwargs):
                self.assertNotIn("SLINKYWINKY_ATTACH_TEST",kwargs["env"])
                self.assertNotIn("SLINKYWINKY_EXPERIMENTAL_HOOK",kwargs["env"])
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_OVERLAY"),"1")
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_VISIBILITY"),"1")
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_MULTIFRAME"),"1")
                if cmd[1]=="list":
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"123 net.minecraft.client.main.Main\n","")
                self.assertEqual(cmd[1],"attach")
                self.assertEqual(cmd[2],"123")
                option=cmd[-1]
                attached.append(option)
                stage={"--render-preflight":P,
                       "--render-callback=8":C,
                       "--render-context=8":W,
                       "--render-overlay=8":O,
                       "--render-overlay=72":M,
                       "--render-visibility=72":V}[option]
                with log.open("a",encoding="utf-8") as output:
                    output.write(stage)
                return harness.subprocess.CompletedProcess(
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+
                    harness.LOADER_MODES[option]+"\n","")
            with patch.object(harness,"process_birth_token",return_value=81), \
                 patch.object(harness,"verify_original_client_jar",return_value=(1,2,3,4)), \
                 patch.object(harness.subprocess,"run",side_effect=fake):
                result=harness.run([
                    "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                    "--pid","123","--target-stdout-log",str(log),
                    "--original-client-jar",str(client),
                    "--mode","visibility","--execute","--report",str(report)])
            self.assertEqual(result,0)
            self.assertEqual(attached,
                             ["--render-preflight","--render-callback=8",
                              "--render-context=8","--render-overlay=8",
                              "--render-overlay=72","--render-visibility=72"])
            data=json.loads(report.read_text(encoding="utf-8"))
            self.assertEqual(len(data["stages"]),6)
            self.assertEqual(data["stages"][-1]["mode"],"--render-visibility=72")
            self.assertFalse(data["visual_game_acceptance"])
            self.assertFalse(data["in_game_menu_verified"])

    @unittest.skipUnless(os.name=="nt", "R94 synthetic operator control requires Windows")
    def test_r94_seven_stage_manual_visibility_and_pid_controls(self):
        # Fully simulated Attach and JVM stdout: NEVER start Minecraft.
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            loader=root/"slinkywinky_loader.exe"
            java=root/"jdk8"/"bin"/"java.exe"
            tools=root/"jdk8"/"lib"/"tools.jar"
            log=root/"target-stdout.log"
            client=root/"fixture-original-client.jar"
            report=root/"r94-evidence.json"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log,client):
                file.touch()
            attached=[]
            def fake(cmd,**kwargs):
                self.assertNotIn("SLINKYWINKY_ATTACH_TEST",kwargs["env"])
                self.assertNotIn("SLINKYWINKY_EXPERIMENTAL_HOOK",kwargs["env"])
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_OVERLAY"),"1")
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_VISIBILITY"),"1")
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_MANUAL"),"1")
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_MULTIFRAME"),"1")
                if cmd[1]=="list":
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"123 net.minecraft.client.main.Main\n","")
                self.assertEqual(cmd[1],"attach")
                self.assertEqual(cmd[2],"123")
                option=cmd[-1]
                attached.append(option)
                if option in ("--toggle-visibility","--stop-visibility"):
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+option[2:]+"\n","")
                stage={"--render-preflight":P,
                       "--render-callback=8":C,
                       "--render-context=8":W,
                       "--render-overlay=8":O,
                       "--render-overlay=72":M,
                       "--render-visibility=72":V,
                       "--render-visibility=manual":U}[option]
                with log.open("a",encoding="utf-8") as output:
                    output.write(stage)
                return harness.subprocess.CompletedProcess(
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+
                    harness.LOADER_MODES[option]+"\n","")
            with patch.object(harness,"process_birth_token",return_value=81), \
                 patch.object(harness,"verify_original_client_jar",return_value=(1,2,3,4)), \
                 patch.object(harness.subprocess,"run",side_effect=fake):
                result=harness.run([
                    "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                    "--pid","123","--target-stdout-log",str(log),
                    "--original-client-jar",str(client),
                    "--mode","manual","--execute","--report",str(report)])
            self.assertEqual(result,0)
            self.assertEqual(attached,
                             ["--render-preflight","--render-callback=8",
                              "--render-context=8","--render-overlay=8",
                              "--render-overlay=72","--render-visibility=72",
                              "--render-visibility=manual",
                              "--toggle-visibility","--toggle-visibility",
                              "--toggle-visibility","--stop-visibility"])
            data=json.loads(report.read_text(encoding="utf-8"))
            self.assertEqual(len(data["stages"]),7)
            self.assertEqual(data["stages"][-1]["mode"],"--render-visibility=manual")
            self.assertFalse(data["visual_game_acceptance"])
            self.assertFalse(data["in_game_menu_verified"])

    @unittest.skipUnless(os.name=="nt", "R95 synthetic original foreground key requires Windows")
    def test_r95_seven_stage_original_only_insert_key_and_stop(self):
        # Fully simulated Attach and JVM stdout: NEVER start Minecraft.
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            loader=root/"slinkywinky_loader.exe"
            java=root/"jdk8"/"bin"/"java.exe"
            tools=root/"jdk8"/"lib"/"tools.jar"
            log=root/"target-stdout.log"
            client=root/"fixture-original-client.jar"
            report=root/"r95-evidence.json"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log,client):
                file.touch()
            attached=[]
            def fake(cmd,**kwargs):
                self.assertNotIn("SLINKYWINKY_ATTACH_TEST",kwargs["env"])
                self.assertNotIn("SLINKYWINKY_EXPERIMENTAL_HOOK",kwargs["env"])
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_OVERLAY"),"1")
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_VISIBILITY"),"1")
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_MANUAL"),"1")
                self.assertEqual(kwargs["env"].get("SLINKYWINKY_RENDER_MULTIFRAME"),"1")
                if cmd[1]=="list":
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"123 net.minecraft.client.main.Main\n","")
                self.assertEqual(cmd[1],"attach")
                self.assertEqual(cmd[2],"123")
                option=cmd[-1]
                attached.append(option)
                if option in ("--toggle-visibility","--stop-visibility"):
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+option[2:]+"\n","")
                stage={"--render-preflight":P,
                       "--render-callback=8":C,
                       "--render-context=8":W,
                       "--render-overlay=8":O,
                       "--render-overlay=72":M,
                       "--render-visibility=72":V,
                       "--render-visibility=manual":K}[option]
                with log.open("a",encoding="utf-8") as output:
                    output.write(stage)
                return harness.subprocess.CompletedProcess(
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+
                    harness.LOADER_MODES[option]+"\n","")
            with patch.object(harness,"process_birth_token",return_value=81), \
                 patch.object(harness,"verify_original_client_jar",return_value=(1,2,3,4)), \
                 patch.object(harness.subprocess,"run",side_effect=fake), \\\n                 patch.object(harness,"send_isolated_insert",return_value=2) as injected:
                result=harness.run([
                    "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                    "--pid","123","--target-stdout-log",str(log),
                    "--original-client-jar",str(client),
                    "--mode","key","--execute","--report",str(report)])
            self.assertEqual(result,0)
            injected.assert_called_once_with(123,root)
            self.assertEqual(attached,
                             ["--render-preflight","--render-callback=8",
                              "--render-context=8","--render-overlay=8",
                              "--render-overlay=72","--render-visibility=72",
                              "--render-visibility=manual","--stop-visibility"])
            data=json.loads(report.read_text(encoding="utf-8"))
            self.assertEqual(len(data["stages"]),7)
            self.assertEqual(data["stages"][-1]["mode"],"--render-visibility=manual")
            self.assertFalse(data["visual_game_acceptance"])
            self.assertFalse(data["in_game_menu_verified"])

    @unittest.skipUnless(os.name=="nt", "R82 simulated failure requires Windows")
    def test_execute_mocked_restoration_failure_never_starts_wgl_stage(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            loader=root/"slinkywinky_loader.exe"
            java=root/"jdk8"/"bin"/"java.exe"
            tools=root/"jdk8"/"lib"/"tools.jar"
            log=root/"target-stdout.log"
            client=root/"fixture-original-client.jar"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log,client):
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
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+
                    harness.LOADER_MODES[option]+"\n","")
            with patch.object(harness,"process_birth_token",return_value=8123456789), \
                 patch.object(harness,"verify_original_client_jar",return_value=(1,2,3,4)), \
                 patch.object(harness.subprocess,"run",side_effect=fake):
                with self.assertRaises(harness.ReceiptError):
                    harness.run([
                        "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                        "--pid","123","--target-stdout-log",str(log),
                    "--original-client-jar",str(client),
                        "--mode","context","--execute"])
            self.assertEqual(attached,["--render-preflight",
                                       "--render-callback=8"])

    @unittest.skipUnless(os.name=="nt", "R84 PID reuse fixture requires Windows")
    def test_pid_recycling_between_preflight_and_callback_refused(self):
        # Both list calls return the SAME PID, yet its kernel birth token
        # changes. Never Attach to the new JVM after preflight has completed.
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            loader=root/"slinkywinky_loader.exe"
            java=root/"jdk8"/"bin"/"java.exe"
            tools=root/"jdk8"/"lib"/"tools.jar"
            log=root/"target-stdout.log"
            client=root/"fixture-original-client.jar"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log,client):
                file.touch()
            attached=[]
            def fake(cmd,**kwargs):
                if cmd[1]=="list":
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"123 net.minecraft.client.main.Main\n","")
                option=cmd[-1]
                attached.append(option)
                with log.open("a",encoding="utf-8") as output:
                    output.write(P)
                return harness.subprocess.CompletedProcess(
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+
                    harness.LOADER_MODES[option]+"\n","")
            # Startup, preflight pre-Attach, post-Attach, post-receipt,
            # then mismatched creation token before callback Attach.
            births=[101,101,101,101,202]
            with patch.object(harness,"process_birth_token",
                              side_effect=births) as check_birth, \
                 patch.object(harness,"verify_original_client_jar",return_value=(1,2,3,4)), \
                 patch.object(harness.subprocess,"run",side_effect=fake):
                with self.assertRaisesRegex(harness.ReceiptError,"replaced/restarted"):
                    harness.run([
                        "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                        "--pid","123","--target-stdout-log",str(log),
                    "--original-client-jar",str(client),
                        "--mode","context","--execute"])
            self.assertEqual(check_birth.call_count,5)
            self.assertEqual(attached,["--render-preflight"])

    def test_local_client_jar_rejects_wrong_size_and_digest(self):
        with tempfile.TemporaryDirectory() as folder:
            client=Path(folder)/"1.8.9.jar"
            client.write_bytes(b"forged")
            with self.assertRaisesRegex(harness.ReceiptError,"original vanilla"):
                harness.verify_original_client_jar(client)
            # Even an exact-length but counterfeit JAR must fail both pins.
            with client.open("wb") as stream:
                stream.truncate(harness.PINNED_VANILLA_189_BYTES)
            with self.assertRaisesRegex(harness.ReceiptError,"digest mismatch"):
                harness.verify_original_client_jar(client)

    @unittest.skipUnless(os.name=="nt", "Simulated live Attach requires Windows")
    def test_original_jar_swap_between_stages_refuses_callback_attach(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            loader=root/"slinkywinky_loader.exe"
            java=root/"jdk8"/"bin"/"java.exe"
            tools=root/"jdk8"/"lib"/"tools.jar"
            log=root/"target-stdout.log"
            client=root/"fixture-original-client.jar"
            java.parent.mkdir(parents=True)
            tools.parent.mkdir(parents=True)
            for file in (loader,java,tools,log,client):
                file.touch()
            attached=[]
            def fake(cmd,**kwargs):
                if cmd[1]=="list":
                    return harness.subprocess.CompletedProcess(
                        cmd,0,"123 net.minecraft.client.main.Main\n","")
                option=cmd[-1]
                attached.append(option)
                with log.open("a",encoding="utf-8") as output:
                    output.write(P)
                return harness.subprocess.CompletedProcess(
                    cmd,0,"ATTACH_RESULT=SUCCESS pid=123 mode="+
                    harness.LOADER_MODES[option]+"\n","")
            same=(1,2,3,4)
            changed=(1,2,3,5)
            # Initial SHA, pre-R73 SHA, post-R73 SHA, then changed pre-R75 SHA.
            with patch.object(harness,"verify_original_client_jar",
                              side_effect=[same,same,same,changed]), \
                 patch.object(harness,"process_birth_token",return_value=101), \
                 patch.object(harness.subprocess,"run",side_effect=fake):
                with self.assertRaisesRegex(harness.ReceiptError,"replaced"):
                    harness.run([
                        "--loader",str(loader),"--jdk8",str(root/"jdk8"),
                        "--pid","123","--target-stdout-log",str(log),
                    "--original-client-jar",str(client),
                        "--mode","context","--execute"])
            self.assertEqual(attached,["--render-preflight"])

    def test_explicit_birth_token_comparison_is_fail_closed(self):
        with patch.object(harness,"process_birth_token",return_value=222):
            harness.require_same_process(123,222)
            with self.assertRaises(harness.ReceiptError):
                harness.require_same_process(123,111)

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
