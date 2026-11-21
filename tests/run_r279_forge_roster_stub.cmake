# R279: isolated Java8 compile of the UNMODIFIED R277 Forge adapter.
if(NOT JAVA_COMPILER OR NOT JAVA_EXECUTABLE OR NOT PROJECT_ROOT OR NOT BUILD_ROOT)
  message(FATAL_ERROR "R279 Java toolchain or worktree missing")
endif()
set(SRC "${BUILD_ROOT}/r279-stubs")
set(OUT "${BUILD_ROOT}/r279-classes")
foreach(PATH net/minecraft/client net/minecraft/client/entity net/minecraft/client/multiplayer net/minecraft/entity/player net/minecraft/item)
  file(MAKE_DIRECTORY "${SRC}/${PATH}")
endforeach()
file(MAKE_DIRECTORY "${OUT}")
file(WRITE "${SRC}/net/minecraft/item/Item.java" [=[
package net.minecraft.item;
public class Item {
  public final int id;
  public Item(int id){this.id=id;}
  public static int getIdFromItem(Item it){return it.id;}
}
]=])
file(WRITE "${SRC}/net/minecraft/item/ItemStack.java" [=[
package net.minecraft.item;
public class ItemStack {
  private final Item item;
  public ItemStack(Item i){item=i;}
  public Item getItem(){return item;}
}
]=])
file(WRITE "${SRC}/net/minecraft/entity/player/InventoryPlayer.java" [=[
package net.minecraft.entity.player;
import net.minecraft.item.ItemStack;
public class InventoryPlayer {
  public final ItemStack[] armorInventory=new ItemStack[4];
}
]=])
file(WRITE "${SRC}/net/minecraft/entity/player/EntityPlayer.java" [=[
package net.minecraft.entity.player;
import java.util.UUID;
public class EntityPlayer {
  public final UUID uuid;
  public final String name;
  public final InventoryPlayer inventory=new InventoryPlayer();
  public double prevPosX,prevPosZ,posX,posZ;
  public int ticksExisted=25;
  public boolean sleeping,invisible;
  public EntityPlayer(UUID id,String n){uuid=id;name=n;}
  public UUID getUniqueID(){return uuid;}
  public String getName(){return name;}
  public boolean isPlayerSleeping(){return sleeping;}
  public boolean isInvisibleToPlayer(EntityPlayer p){return invisible;}
}
]=])
file(WRITE "${SRC}/net/minecraft/client/entity/EntityPlayerSP.java" [=[
package net.minecraft.client.entity;
import java.util.UUID;
import net.minecraft.entity.player.EntityPlayer;
public class EntityPlayerSP extends EntityPlayer {
  public EntityPlayerSP(UUID id,String n){super(id,n);}
}
]=])
file(WRITE "${SRC}/net/minecraft/client/multiplayer/WorldClient.java" [=[
package net.minecraft.client.multiplayer;
import java.util.*;
import net.minecraft.entity.player.EntityPlayer;
public class WorldClient {
  public boolean isRemote=true;
  public final List<EntityPlayer> playerEntities=new ArrayList<EntityPlayer>();
}
]=])
file(WRITE "${SRC}/net/minecraft/client/Minecraft.java" [=[
package net.minecraft.client;
import net.minecraft.client.entity.EntityPlayerSP;
import net.minecraft.client.multiplayer.WorldClient;
public class Minecraft {
  public WorldClient theWorld;
  public EntityPlayerSP thePlayer;
  public boolean singleplayer=true;
  public boolean isSingleplayer(){return singleplayer;}
}
]=])
execute_process(COMMAND "${JAVA_COMPILER}" -source 8 -target 8 -d "${OUT}"
  "${SRC}/net/minecraft/item/Item.java"
  "${SRC}/net/minecraft/item/ItemStack.java"
  "${SRC}/net/minecraft/entity/player/InventoryPlayer.java"
  "${SRC}/net/minecraft/entity/player/EntityPlayer.java"
  "${SRC}/net/minecraft/client/entity/EntityPlayerSP.java"
  "${SRC}/net/minecraft/client/multiplayer/WorldClient.java"
  "${SRC}/net/minecraft/client/Minecraft.java"
  "${PROJECT_ROOT}/java/dev/slinkywinky/RelationshipRosterSampler.java"
  "${PROJECT_ROOT}/experimental/forge189/src/main/java/dev/slinkywinky/forge/ForgeRelationshipRosterPort.java"
  "${PROJECT_ROOT}/tests/java/dev/slinkywinky/forge/ForgeRelationshipRosterPortSmoke.java"
  RESULT_VARIABLE RC OUTPUT_VARIABLE STDOUT ERROR_VARIABLE STDERR)
if(NOT RC EQUAL 0)
  message(FATAL_ERROR "R279 Java8 stub compile refused: ${STDOUT} ${STDERR}")
endif()
execute_process(COMMAND "${JAVA_EXECUTABLE}" -cp "${OUT}"
  dev.slinkywinky.forge.ForgeRelationshipRosterPortSmoke
  RESULT_VARIABLE RC OUTPUT_VARIABLE STDOUT ERROR_VARIABLE STDERR)
if(NOT RC EQUAL 0 OR NOT STDOUT MATCHES "R279_FORGE_ROSTER_STUB_SOURCE_PASS")
  message(FATAL_ERROR "R279 roster Port test refused: ${STDOUT} ${STDERR}")
endif()
message(STATUS "${STDOUT}")
