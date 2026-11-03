package dev.slinkywinky.attach;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.Handle;
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R72: a memory-only ASM preflight on the SHA-pinned original bfk.class.
 *
 * No Instrumentation, class definition, Java Attach, JNI, OpenGL or game launch.
 * The deliberately unresolved marker reference is NEVER executed or shipped in
 * the injectable agent. Transformed proprietary bytes are NEVER written out.
 */
public final class OriginalRendererWeaveAudit implements Opcodes {
    private static final String TARGET_CLASS = "bfk";
    private static final String TARGET_METHOD = "a";
    private static final String TARGET_DESC = "(FJ)V";
    private static final String MARKER = "dev/slinkywinky/r72/OfflineOnlyMarker";
    private static final String VERIFIED_BFK_SHA256 =
        "8a4ce93bbf272398ea298d2ed363f75211dd469fcd3b66682a387e4738a419a2";

    private static void require(boolean allowed, String problem) {
        if (!allowed) throw new IllegalArgumentException(problem);
    }

    private static final class Snapshot {
        String className;
        final List<String> methods = new ArrayList<String>();
        final List<String> instructions = new ArrayList<String>();
        int targets;
        int markers;

        String methodKey(int access, String name, String desc) {
            return access + ":" + name + desc;
        }
    }

    private static Snapshot snapshot(byte[] bytes) {
        final Snapshot result = new Snapshot();
        ClassReader reader = new ClassReader(bytes);
        result.className = reader.getClassName();
        reader.accept(new ClassVisitor(ASM5) {
            @Override public MethodVisitor visitMethod(int access, String name,
                    String desc, String sig, String[] exceptions) {
                result.methods.add(result.methodKey(access,name,desc));
                if (!TARGET_METHOD.equals(name) || !TARGET_DESC.equals(desc))
                    return null;
                result.targets++;
                return new MethodVisitor(ASM5) {
                    private void add(String value) { result.instructions.add(value); }
                    @Override public void visitInsn(int opcode) {
                        add("I:" + opcode);
                    }
                    @Override public void visitIntInsn(int opcode, int operand) {
                        add("N:" + opcode + ":" + operand);
                    }
                    @Override public void visitVarInsn(int opcode, int var) {
                        add("V:" + opcode + ":" + var);
                    }
                    @Override public void visitTypeInsn(int opcode, String type) {
                        add("T:" + opcode + ":" + type);
                    }
                    @Override public void visitFieldInsn(int opcode, String owner,
                            String field, String descriptor) {
                        add("F:" + opcode + ":" + owner + ":" + field + ":" + descriptor);
                    }
                    @Override public void visitMethodInsn(int opcode, String owner,
                            String method, String descriptor, boolean itf) {
                        String event="M:" + opcode + ":" + owner + ":" + method +
                                     ":" + descriptor + ":" + itf;
                        add(event);
                        if (opcode==INVOKESTATIC && MARKER.equals(owner))
                            result.markers++;
                    }
                    @Override public void visitInvokeDynamicInsn(String name,
                            String desc, Handle bootstrap, Object... args) {
                        add("D:" + name + ":" + desc + ":" + bootstrap +
                            ":" + Arrays.deepToString(args));
                    }
                    @Override public void visitJumpInsn(int opcode, Label label) {
                        add("J:" + opcode);
                    }
                    @Override public void visitLdcInsn(Object value) {
                        add("L:" + String.valueOf(value));
                    }
                    @Override public void visitIincInsn(int variable, int increment) {
                        add("C:" + variable + ":" + increment);
                    }
                    @Override public void visitTableSwitchInsn(int min, int max,
                            Label dflt, Label... labels) {
                        add("S:" + min + ":" + max + ":" + labels.length);
                    }
                    @Override public void visitLookupSwitchInsn(Label dflt,
                            int[] keys, Label[] labels) {
                        add("K:" + Arrays.toString(keys));
                    }
                    @Override public void visitMultiANewArrayInsn(String desc, int dims) {
                        add("A:" + desc + ":" + dims);
                    }
                };
            }
        }, ClassReader.SKIP_DEBUG | ClassReader.SKIP_FRAMES);
        return result;
    }

    /**
     * Package-private injection-proof test seam. The CLI always supplies the
     * pinned Mojang digest; synthetic tests supply their own generated digest.
     */
    static void checkWeave(byte[] original, String expectedSha256) throws Exception {
        require(original!=null && original.length>0 && original.length<=1000000,
            "Missing or oversized renderer class");
        require(RenderCallsiteAudit.sha256Hex(original).equals(expectedSha256),
            "Renderer SHA-256 mismatch");
        final Snapshot before=snapshot(original);
        require(TARGET_CLASS.equals(before.className) && before.targets==1 &&
                before.markers==0, "Wrong or ambiguous original renderer");
        final ClassReader reader=new ClassReader(original);
        final ClassWriter writer=new ClassWriter(reader, ClassWriter.COMPUTE_MAXS);
        final int[] amended={0};
        reader.accept(new ClassVisitor(ASM5,writer) {
            @Override public MethodVisitor visitMethod(int access, String name,
                    String desc, String sig, String[] exceptions) {
                MethodVisitor delegate=super.visitMethod(access,name,desc,sig,exceptions);
                if(!TARGET_METHOD.equals(name) || !TARGET_DESC.equals(desc))
                    return delegate;
                require((access & ACC_PUBLIC)!=0 &&
                        (access & (ACC_STATIC|ACC_ABSTRACT|ACC_NATIVE))==0,
                        "Render method is not a public concrete instance method");
                amended[0]++;
                return new MethodVisitor(ASM5,delegate) {
                    @Override public void visitCode() {
                        super.visitCode();
                        super.visitVarInsn(FLOAD,1);
                        super.visitVarInsn(LLOAD,2);
                        super.visitMethodInsn(INVOKESTATIC,MARKER,"touch",TARGET_DESC,false);
                    }
                };
            }
        },0);
        require(amended[0]==1, "Expected one patched renderer method");
        // No filesystem output: the modified bytes remain local to this method.
        final byte[] changed=writer.toByteArray();
        final Snapshot after=snapshot(changed);
        final List<String> expected=new ArrayList<String>(before.instructions);
        expected.add(0, "M:184:" + MARKER + ":touch:" + TARGET_DESC + ":false");
        expected.add(0, "V:22:2"); // LLOAD 2
        expected.add(0, "V:23:1"); // FLOAD 1
        require(TARGET_CLASS.equals(after.className) && after.targets==1 &&
                after.markers==1 && before.methods.equals(after.methods) &&
                expected.equals(after.instructions),
                "ASM transformation changed original method instructions");
        require(RenderCallsiteAudit.sha256Hex(original).equals(expectedSha256),
                "Read-only original byte array changed");
    }

    public static void main(String[] args) throws Exception {
        require(args.length==1, "Usage: OriginalRendererWeaveAudit ORIGINAL_1_8_9_JAR");
        Path jar=Paths.get(args[0]);
        require(jar.isAbsolute() && Files.isRegularFile(jar),
                "Original JAR must be an absolute existing file");
        // Reuses R64/R71 original whole-JAR SHA-1, file-size and structural
        // uniqueness admission before inspecting renderer class in memory.
        RenderCallsiteAudit.main(args);
        ZipFile archive=new ZipFile(jar.toFile());
        try {
            byte[] original=RenderCallsiteAudit.entry(archive,TARGET_CLASS+".class");
            checkWeave(original,VERIFIED_BFK_SHA256);
        } finally {archive.close();}
        System.out.println("R72_ORIGINAL_BFK_OFFLINE_WEAVE_PREFLIGHT_PASS");
        System.out.println("R72_NO_GAME_ATTACH_OR_TRANSFORMED_BINARY_OUTPUT");
    }
    private OriginalRendererWeaveAudit() {}
}
