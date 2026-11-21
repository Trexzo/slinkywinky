package dev.slinkywinky.forge;
import dev.slinkywinky.RelationshipRosterSampler;
import java.util.UUID;
import net.minecraft.client.Minecraft;
import net.minecraft.client.entity.EntityPlayerSP;
import net.minecraft.client.multiplayer.WorldClient;
import net.minecraft.entity.player.EntityPlayer;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
public final class ForgeRelationshipRosterPortSmoke {
  private static void check(boolean yes,String why){
    if(!yes)throw new AssertionError("R279 "+why);
  }
  private static void denied(Runnable r,String why){
    boolean caught=false;
    try{r.run();}catch(IllegalStateException expected){caught=true;}
    check(caught,why);
  }
  public static void main(String[] args){
    final Minecraft mc=new Minecraft();
    final EntityPlayerSP local=new EntityPlayerSP(new UUID(0,99),"Local");
    final EntityPlayer other=new EntityPlayer(new UUID(0x2000,1),"Same");
    final EntityPlayer another=new EntityPlayer(new UUID(0x5000,2),"Same");
    other.prevPosX=0;other.posX=2;other.invisible=true;
    other.inventory.armorInventory[0]=new ItemStack(new Item(276));
    mc.theWorld=new WorldClient();mc.thePlayer=local;
    mc.theWorld.playerEntities.add(local);
    mc.theWorld.playerEntities.add(other);
    mc.theWorld.playerEntities.add(another);
    final RelationshipRosterSampler s=new RelationshipRosterSampler();
    final ForgeRelationshipRosterPort port=new ForgeRelationshipRosterPort(mc);
    RelationshipRosterSampler.Receipt r=s.observe(port,1);
    check(r.remote.size()==2 && r.remote.get(0).uuid.equals(other.uuid)
      && r.remote.get(1).uuid.equals(another.uuid),"distinct UUID roster");
    check(r.remote.get(0).armor && r.remote.get(0).invisible
      && r.remote.get(0).x==2,"source armor/movement/invisibility");
    mc.theWorld=new WorldClient();
    mc.theWorld.playerEntities.add(local);
    RelationshipRosterSampler.Receipt n=s.observe(port,0);
    check(n.generation==2 && n.worldToken!=r.worldToken,"world owner change");
    mc.singleplayer=false;
    denied(new Runnable(){public void run(){s.observe(port,1);}},
      "public multiplayer refused");
    mc.singleplayer=true;mc.theWorld.isRemote=false;
    denied(new Runnable(){public void run(){s.observe(port,1);}},
      "server world refused");
    s.close();
    System.out.println("R279_FORGE_ROSTER_STUB_SOURCE_PASS");
  }
}
