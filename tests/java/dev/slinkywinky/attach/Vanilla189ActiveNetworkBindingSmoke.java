package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.Vanilla189ActiveNetworkBinding;
import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;

/** R368 original-name manager identity, weak ownership, zero network I/O. */
public final class Vanilla189ActiveNetworkBindingSmoke {
    static void yes(boolean value,String what){if(!value)throw new AssertionError(what);}
    static Object instance(String name)throws Exception{
        Constructor<?> c=Class.forName(name).getDeclaredConstructor();
        c.setAccessible(true);return c.newInstance();
    }
    static void put(Object object,String field,Object value)throws Exception{
        Field f=object.getClass().getDeclaredField(field);
        f.setAccessible(true);f.set(object,value);
    }
    static void denied(Runnable r,String text){
        boolean rejected=false;
        try{r.run();}catch(IllegalArgumentException|IllegalStateException ex){
            rejected=true;
        }
        yes(rejected,text);
    }
    public static void main(String[] args)throws Exception{
        final Class<?> ave=Class.forName("ave");
        final Class<?> bcy=Class.forName("bcy");
        final Class<?> ek=Class.forName("ek");
        Object minecraft=instance("ave");
        Vanilla189ActiveNetworkBinding binding=
            new Vanilla189ActiveNetworkBinding(ave,bcy,ek);
        final Object first=binding.read(minecraft);
        yes(first!=null&&first.getClass()==ek,
            "Original ave.u -> bcy.c -> ek instance resolved");
        WeakReference<Object> onlyWeak=new WeakReference<Object>(first);
        yes(onlyWeak.get()==first,"Weak manager identity preserves exact reference");
        Object handler=instance("bcy");
        put(minecraft,"handler",handler);
        final Object second=binding.read(minecraft);
        yes(second!=null&&second!=first,
            "Replacing world network handler revokes previous manager identity");
        yes(onlyWeak.get()!=second,"Retired connection cannot match current manager");
        put(minecraft,"handler",null);
        yes(binding.read(minecraft)==null,"Disconnected NetHandler has no manager");
        put(minecraft,"handler",instance("bcy"));
        yes(binding.read(minecraft)!=null,
            "Reconnect returns only the new source network manager");
        denied(()->{
            try{binding.read(new Object());}
            catch(ReflectiveOperationException ex){throw new IllegalStateException(ex);}
        },"Non-vanilla client owner rejected");
        binding.close();
        denied(()->{
            try{binding.read(minecraft);}
            catch(ReflectiveOperationException ex){throw new IllegalStateException(ex);}
        },"Retired reflection classes cannot retain network identities");
        System.out.println("R368_ORIGINAL_NETWORK_MANAGER_WEAK_IDENTITY_GATE_PASS");
    }
}
