#!/usr/bin/env python3
"""R258 Java8 mapped read-only client-thread fixture -> C++ metadata ABI.

Runs only in Java8 disposable mapped fixture; native event-8 gameplay
inputs are intentionally unknown and no Minecraft process is launched.
"""
import pathlib,subprocess,sys
def main():
    if len(sys.argv)!=5:raise ValueError("java, testclasses, instrument.jar, native probe")
    java,classes,jar,probe=sys.argv[1:]
    for path in (java,jar,probe):
        if not pathlib.Path(path).is_file():raise FileNotFoundError(path)
    cmd=[java,"-cp",classes+";"+jar,
         "dev.slinkywinky.attach.R258Event8ObservationSmoke"]
    result=subprocess.run(cmd,text=True,capture_output=True,timeout=35)
    if result.returncode:
        raise AssertionError("R258 Java source fixture failed:\n"+result.stdout+
                             "\n"+result.stderr)
    frames=[x for x in result.stdout.splitlines()
            if x.startswith("R258_EVENT8_OBS ")]
    if len(frames)!=4 or "R258_JAVA8_MAPPED_SOURCE_OBSERVATION_AND_LIFECYCLE_PASS" not in result.stdout:
        raise AssertionError("R258 forged or missing Java receipts "+result.stdout)
    native=subprocess.run([probe]+frames,text=True,capture_output=True,timeout=20)
    if native.returncode or "R258_REAL_JAVA8_OBSERVED_FRAME_NATIVE_ABI_PASS" not in native.stdout:
        raise AssertionError("R258 C++ read-only ABI refused source receipts\n"+
                             native.stdout+"\n"+native.stderr)
    print("R258_JAVA8_MAPPED_SAMPLER_TO_NATIVE_EVENT8_METADATA_PASS")
    print("R258_REAL_MODULE_EVIDENCE_STILL_MISSING_NO_GAMEPLAY_WRITE_PASS")
if __name__=="__main__":main()
