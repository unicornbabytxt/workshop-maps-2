# Workshop Maps 2.0 (BakkesMod plugin)

Install and play Rocket League workshop maps without the console. Type a map number, hit Install, hit Play.

## Install the plugin (one time)

1. Download `WorkshopMaps2.dll` from the Releases page of this repo.
2. In BakkesMod: File > Open BakkesMod folder. Drop the dll in the `plugins` folder.
3. Start Rocket League **with anti-cheat disabled** (Epic: the "..." next to the game > Run without Easy Anti-Cheat).
4. Press F6 and type: `plugin load workshopmaps2`. You only do this once, after that it loads by itself.

## Use it

- Press F2 > Plugins > Workshop Maps 2.0 > Open Workshop Maps. Or bind a key to `togglemenu workshopmaps2`.
- Find a map at https://bakkesplugins.com/maps. The number at the end of the page address is the map number.
- Type the number (or paste the whole link), hit Install, then hit Play next to the map.

Maps go in `<Rocket League>\TAGame\CookedPCConsole\mods` by default. Anything already in there shows up in the list. You can change the folder under Settings.

If the number install can't reach the site, download the zip in your browser and use "Install from a zip in my Downloads folder" in the plugin.

Console shortcuts: `wm_install 298`, `wm_open`.

## Free play mode

Turn on "Free play mode" above the map list and maps load as Free Play, with a ball and your normal free play bindings.

It doesn't work on every map. It's best on maps that play like a normal arena. Maps with their own mini games or checkpoints use the reset shot button themselves, and Free Play takes that button over, so turn it off for those.

## Updates

When a newer build is out, an "Update now" button shows at the top of the plugin. Hit it, then restart Rocket League.

## Workshop textures

Most maps need the shared workshop textures or they look black or pink. The plugin checks for them every time it opens. If they're missing, hit "Get textures", download the zip in your browser, and the plugin installs it from your Downloads folder by itself. It also spots texture files that ended up in the wrong folder (the cause of "Ambiguous package name" popups) and fixes them in one click.

## Play with friends

Open the Play with Friends tab.

- Host: pick a map, hit Host this map, hit Find my IP, then Copy code and send it to your friends. Hit "Open my router port automatically" once so they can reach you.
- Friends: paste the code and hit Join. The plugin installs the map first if they don't have it.

Easiest way with no router work: everyone installs a free virtual LAN app (Radmin VPN, ZeroTier, Tailscale, or Hamachi) and joins the same network in it. The plugin finds your address in that app and you hit "Use this address".

If you'd rather not, hit "Open my router port automatically", or forward UDP port 7777 to the host PC.

## Notes

- Two copies of the same map in the maps folder can crash the game. The plugin flags duplicates and has a one click cleanup that moves the extra copies to a backup folder.
- Workshop maps only work with anti-cheat disabled, which means offline/private play. That is how Rocket League is set up, not something this plugin changes.
- Some maps need the workshop textures pack in `CookedPCConsole` or they look blank/pink.

## Support

The plugin is free and always will be. If it made your life easier and you want to throw a couple bucks my way: https://buymeacoffee.com/unicornbaby.txt
