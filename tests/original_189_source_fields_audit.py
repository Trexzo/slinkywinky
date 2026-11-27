#!/usr/bin/env python3
"""R357: verify source-candidate obfuscated fields in hash-pinned Mojang 1.8.9.

Read-only classfile metadata only. Never exports or writes original JAR bytes.
"""
import hashlib
import pathlib
import struct
import zipfile

MOJANG_189_SHA1 = "3870888a6c3d349d3771a3e9d16c9bf5e076b908"
MOJANG_189_SIZE = 8461484
# Original candidate ptr_to_str_sv / ptr_to_str_bll, variant=1.
# Only these NAME+TYPE facts are admitted, not source module behavior.
EXPECTED = {
    "pr": {"bl": "I"},                   # EntityLivingBase lastAttackerTime
    "bex": {"c": "D", "d": "D", "e": "D"}, # EntityOtherPlayerMP xyz
}


def class_fields(raw):
    """Minimal bounds-checked JVM ClassFile metadata decoder; no bytecode."""
    data = memoryview(raw)
    offset = 0

    def take(n):
        nonlocal offset
        if n < 0 or offset + n > len(data):
            raise ValueError("Truncated or oversized Java class metadata")
        out = data[offset:offset + n]
        offset += n
        return out

    def u1():
        return take(1)[0]

    def u2():
        return struct.unpack(">H", take(2))[0]

    def u4():
        return struct.unpack(">I", take(4))[0]

    if u4() != 0xCAFEBABE:
        raise ValueError("Not a classfile")
    u2()  # minor
    major = u2()
    if major < 45 or major > 52:
        raise ValueError("Unexpected classfile version")
    count = u2()
    if count < 5 or count > 65535:
        raise ValueError("Bad class constant pool")
    cp = [None] * count
    i = 1
    while i < count:
        tag = u1()
        if tag == 1:  # CONSTANT_Utf8 (modified UTF8; names are ASCII)
            size = u2()
            cp[i] = ("utf8", bytes(take(size)).decode("utf-8", "replace"))
        elif tag in (3, 4):  # Integer, Float
            take(4)
        elif tag in (5, 6):  # Long, Double; occupies two slots
            take(8)
            i += 1
        elif tag == 7:  # Class
            cp[i] = ("class", u2())
        elif tag in (8, 16, 19, 20):  # String/MethodType/Module/Package
            take(2)
        elif tag in (9, 10, 11, 12, 17, 18):
            take(4)
        elif tag == 15:
            take(3)
        else:
            raise ValueError("Unsupported JVM constant pool tag")
        i += 1

    def utf8(index):
        if index <= 0 or index >= count or cp[index] is None or cp[index][0] != "utf8":
            raise ValueError("Untrusted class UTF8 index")
        return cp[index][1]

    u2()  # flags
    owner = u2()
    u2()  # super
    if owner <= 0 or owner >= count or cp[owner] is None or cp[owner][0] != "class":
        raise ValueError("Invalid this_class")
    class_name = utf8(cp[owner][1])
    for _ in range(u2()):  # interfaces
        u2()
    fields = {}
    for _ in range(u2()):
        u2()  # access flags
        name = utf8(u2())
        descriptor = utf8(u2())
        if name in fields:
            raise ValueError("Duplicate Java class field")
        fields[name] = descriptor
        for _ in range(u2()):  # field attributes
            u2()  # name index
            take(u4())
    return class_name, fields


def audit_entries(read_class):
    """Read only two exact owner classes, reject missing/wrong field types."""
    for owner, required in EXPECTED.items():
        name, fields = class_fields(read_class(owner + ".class"))
        if name != owner:
            raise ValueError("Unexpected source class owner: " + owner)
        for field, descriptor in required.items():
            if fields.get(field) != descriptor:
                raise ValueError(
                    "Original Mojang private field mismatch: " + owner + "." + field
                )


def audit_official_source_fields(client_jar):
    path = pathlib.Path(client_jar)
    if path.stat().st_size != MOJANG_189_SIZE:
        raise ValueError("Wrong official game JAR length")
    sha = hashlib.sha1()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(65536), b""):
            sha.update(chunk)
    if sha.hexdigest() != MOJANG_189_SHA1:
        raise ValueError("Wrong original Mojang 1.8.9 SHA-1")
    with zipfile.ZipFile(path, "r") as game:
        audit_entries(game.read)
    print("R357_ORIGINAL_189_PRIVATE_FIELDS_STATIC_PASS "
          "EntityLivingBase=pr.bl:I OtherPlayerMP=bex.cde:D")
    print("R357_NO_GAME_PROCESS_NO_JAR_OR_CLASS_EXPORT")
