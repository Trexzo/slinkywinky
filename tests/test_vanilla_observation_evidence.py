#!/usr/bin/env python3
"""R60 negative controls for isolated vanilla 1.8.9 evidence parsing. Never runs Minecraft."""
import unittest
from windows_vanilla189_isolated_manual import (
    validate_observation_evidence, original_render_classes_loaded
)


class RuntimeEvidenceTests(unittest.TestCase):
    SHA = "a" * 64
    ALT = "b" * 64

    @classmethod
    def record(cls, accepted, world, ray, generations=0, sha=None, cleanup=True):
        fields = (
            "NATIVE_OBSERVATION_SUMMARY accepted={} worldPlayer={} "
            "rayEntity={} generations={}\n"
        ).format(accepted, world, ray, generations)
        fields += "CLASS_BYTECODE_RESTORE_PASS sha256={}\n".format(sha or cls.SHA)
        if cleanup:
            fields += "GAME_THREAD_HOOK_CLEANUP_PASS\n"
        return fields

    def test_r90_exact_verbose_renderer_readiness(self):
        vanilla = ("[Loaded ave from file:/C:/User/.minecraft/versions/1.8.9/1.8.9.jar]\n"
                   "[Loaded bfk from file:/C:/User/.minecraft/versions/1.8.9/1.8.9.jar]\n"
                   "[10:00:00] [Client thread/INFO]: LWJGL Version: 2.9.4\n")
        self.assertTrue(original_render_classes_loaded(vanilla))
        invalid = (
            vanilla.replace("Loaded bfk", "Loaded bfkx"),
            vanilla.replace("Loaded ave", "Loaded aver"),
            vanilla.replace("1.8.9.jar", "1.8.8.jar"),
            vanilla.replace("LWJGL Version:", "Starting LWJGL"),
            vanilla.replace("[Loaded ave from", "[INFO] [Loaded ave from"),
            vanilla.replace("from file:/", "from jar:file:/"),
            vanilla.replace("1.8.9.jar]", "1.8.9.jar.fake]"),
        )
        for candidate in invalid:
            with self.subTest(candidate=candidate):
                self.assertFalse(original_render_classes_loaded(candidate))

    def test_menu_double_attach_compatible_with_r48(self):
        log = self.record(20, 0, 0) + self.record(20, 0, 0)
        evidence, sha = validate_observation_evidence(log, 2, 20)
        self.assertEqual(len(evidence), 2)
        self.assertEqual(sha, [self.SHA, self.SHA])

    def test_world_success_requires_actual_observed_frames(self):
        log = self.record(600, 512, 0, 3)
        evidence, _ = validate_observation_evidence(log, 1, 600, require_world=True)
        self.assertEqual(evidence[0][1], "512")

    def test_ray_entity_success_requires_actual_observed_frames(self):
        log = self.record(600, 600, 25)
        evidence, _ = validate_observation_evidence(
            log, 1, 600, require_world=True, require_ray_entity=True
        )
        self.assertEqual(evidence[0][2], "25")

    def test_menu_only_never_passes_world(self):
        with self.assertRaisesRegex(RuntimeError, "menu-only"):
            validate_observation_evidence(
                self.record(600, 0, 0), 1, 600, require_world=True
            )

    def test_world_without_crosshair_entity_never_passes_ray(self):
        with self.assertRaisesRegex(RuntimeError, "ray-selected"):
            validate_observation_evidence(
                self.record(600, 600, 0), 1, 600, True, True
            )

    def test_partial_callback_count_refused(self):
        with self.assertRaisesRegex(RuntimeError, "callback count"):
            validate_observation_evidence(self.record(599, 599, 25), 1, 600, True, True)

    def test_impossible_world_ray_and_generation_counts_refused(self):
        for log in (
            self.record(600, 601, 1),
            self.record(600, 5, 6),
            self.record(600, 600, 1, 600),
        ):
            with self.subTest(log=log), self.assertRaisesRegex(RuntimeError, "Inconsistent"):
                validate_observation_evidence(log, 1, 600, True)

    def test_restoration_missing_or_mismatched_refused(self):
        with self.assertRaisesRegex(RuntimeError, "fingerprint"):
            validate_observation_evidence("NATIVE_OBSERVATION_SUMMARY accepted=20 "
                                          "worldPlayer=0 rayEntity=0 generations=0\n", 1, 20)
        with self.assertRaisesRegex(RuntimeError, "fingerprint"):
            validate_observation_evidence(
                self.record(20, 0, 0, sha=self.SHA) +
                self.record(20, 0, 0, sha=self.ALT), 2, 20
            )

    def test_cleanup_missing_refused(self):
        with self.assertRaisesRegex(RuntimeError, "cleanup"):
            validate_observation_evidence(self.record(20, 0, 0, cleanup=False), 1, 20)

    def test_rejection_marker_invalidates_otherwise_good_observation(self):
        for marker in (
            "GAME_THREAD_HOOK_ABORT", "NATIVE_GAME_THREAD_PROBE_REJECTED",
            "GAME_THREAD_HOOK_CLEANUP_FAIL", "NATIVE_SESSION_CLOSE_REJECTED"
        ):
            with self.subTest(marker=marker), self.assertRaisesRegex(RuntimeError, "failure"):
                validate_observation_evidence(self.record(600, 600, 1) + marker, 1, 600, True)

    def test_continuous_stop_world_acceptance(self):
        log = self.record(73, 71, 4, 1)
        evidence, sha = validate_observation_evidence(
            log, 1, None, require_world=True, require_ray_entity=True
        )
        self.assertEqual(evidence[0][0], "73")
        self.assertEqual(sha, [self.SHA])

    def test_continuous_must_exceed_short_default_and_stay_below_cap(self):
        for accepted in (0, 20, 36000, 36001):
            with self.subTest(accepted=accepted), self.assertRaisesRegex(
                RuntimeError, "safe range"
            ):
                validate_observation_evidence(
                    self.record(accepted, accepted, 0), 1, None, require_world=True
                )

    def test_continuous_menu_only_does_not_pass_world(self):
        with self.assertRaisesRegex(RuntimeError, "menu-only"):
            validate_observation_evidence(
                self.record(80, 0, 0), 1, None, require_world=True
            )

    def test_duplicate_or_missing_summaries_rejected(self):
        with self.assertRaisesRegex(RuntimeError, "exactly one"):
            validate_observation_evidence(self.record(20, 0, 0) * 2, 1, 20)
        with self.assertRaisesRegex(RuntimeError, "exactly one"):
            validate_observation_evidence(self.SHA, 1, 20)


if __name__ == "__main__":
    unittest.main()
