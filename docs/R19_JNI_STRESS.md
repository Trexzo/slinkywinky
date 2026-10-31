# R19 native JNI render-frequency regression

The real Java 8 JNI bridge now replays 25,000 synthetic frames spaced at 8ms (125fps) through five world identity generations.

The test validates output-edge enum bounds, finite rotations, complete decision delivery and synthetic input release between world changes and shutdown.

The test runs in the existing JDK/JNI CTest smoke job on Linux and Windows. It does **not** measure in-game FPS or claim any gameplay parity.
