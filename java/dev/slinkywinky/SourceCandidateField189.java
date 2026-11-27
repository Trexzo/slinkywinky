package dev.slinkywinky;

import java.lang.reflect.Field;

/**
 * R355: source-candidate Minecraft 1.8.9 private field mappings.
 * Only names confirmed in the private candidate JNI table are accepted.
 * Mapping versions MUST NOT be mixed with the older candidate variant.
 * Missing, wrong-type or ambiguous fields always return null.
 */
public final class SourceCandidateField189 {
    private SourceCandidateField189(){}

    private static Field exact(Class<?> owner,Class<?> expected,
                               String mcp,String srg,String obf){
        if(owner==null||expected==null)throw new IllegalArgumentException("Invalid field owner");
        Field selected=null;
        for(String name:new String[]{mcp,srg,obf}){
            try{
                Field f=owner.getDeclaredField(name);
                if(f.getType()!=expected)return null;
                if(selected!=null)return null; // mixed namespace: refuse
                f.setAccessible(true);
                selected=f;
            }catch(NoSuchFieldException unavailable){
                // Try exactly the next candidate JNI mapping.
            }catch(SecurityException|RuntimeException forbidden){
                return null;
            }
        }
        return selected;
    }

    /** Source JNI ptr_to_str_bo variant=1: lastAttackerTime I. */
    public static Field lastAttackerTime(Class<?> livingBase){
        return exact(livingBase,Integer.TYPE,
            "lastAttackerTime","field_142016_bo","bl");
    }
    /** Source JNI ptr_to_str_d... variant=1: X D. */
    public static Field remoteX(Class<?> otherPlayer){
        return exact(otherPlayer,Double.TYPE,
            "otherPlayerMPX","field_71185_c","c");
    }
    /** Source JNI ptr_to_str_e... variant=1: Y D. */
    public static Field remoteY(Class<?> otherPlayer){
        return exact(otherPlayer,Double.TYPE,
            "otherPlayerMPY","field_71182_d","d");
    }
    /** Source JNI ptr_to_str_f... variant=1: Z D. */
    public static Field remoteZ(Class<?> otherPlayer){
        return exact(otherPlayer,Double.TYPE,
            "otherPlayerMPZ","field_71183_e","e");
    }
}
