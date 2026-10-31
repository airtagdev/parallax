# Parity ledger

Target: Java Edition 26.3. No aggregate completion percentage is assigned.

| Area | Implementation | Evidence / remaining gaps |
| --- | --- | --- |
| Resource import | Implemented for local assets/declarative data | Version and cache hashes validated; shaders/game code excluded; runtime resource-pack precedence missing |
| JSON data reader | Implemented | Parses the prepared recipe/advancement catalogs; grammar and Unicode tests pass; execution of definitions missing |
| English bitmap font | Implemented subset | Seven widget/text regions match native pixels; unifont, format codes and full locale support missing |
| Title main buttons | Implemented rendering | Seven regions exact at 1440x900 / GUI scale 3 / default English assets / non-hover state; other scales/states not verified |
| Title background/logo | Partly implemented | Original textures used; exact panorama movement/filtering, splash/fade/appearance icon and complete compositing unverified or absent |
| Title actions and other menus | Missing | Quit works; other actions explicitly report missing implementation; no custom world substitute |
| java.util.Random | Implemented subset | 4,096 mixed sequences compared against Java; Gaussian and other vanilla RNGs missing |
| Player simulation/input | Missing | Old sandbox approximation retired; no native movement claim |
| Terrain, dimensions, structures | Missing | Old finite generator retired; same-seed generation not implemented |
| NBT/Anvil and world selection | Missing | Old .pxw saves are not native saves |
| Blocks/models/rendering/light | Missing | No native world rendering claim |
| Inventory/items/recipes/workstations | Missing execution | 2,042 original recipe definitions parsed; no recipe execution claim |
| Mobs/AI/combat/physics | Missing | Old box mobs/steering/nutrition retired |
| Advancements | Missing execution/GUI | 1,866 definitions, 126 displays and 4,045 criteria parsed; no trigger/reward/unlock claim |
| Sound/music | Missing playback | Original sound assets prepared locally; generated prototype tones retired |
| Networking/authentication | Missing | Reference offline session is separate; C runtime does not launch Java or perform account authentication |

Next work must replace Missing entries with independently implemented, reference-tested behavior. Cosmetic similarity alone does not change an entry to verified.
