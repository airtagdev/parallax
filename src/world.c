#include "parallax.h"
#include <shlobj.h>

unsigned char *blocks;
WorldState world;
const char *block_names[BLOCK_COUNT] = {
    "Air",      "Grass",       "Dirt",  "Stone", "Sand",   "Oak Log", "Leaves",  "Oak Planks", "Coal Ore",
    "Iron Ore", "Diamond Ore", "Water", "Snow",  "Bricks", "Glass",   "Bedrock", "Lantern"};
const Achievement achievements[ACH_COUNT] = {{"A new horizon", "Create or enter a world"},
                                             {"First steps", "Walk 50 blocks"},
                                             {"Getting wood", "Break an oak log"},
                                             {"Earth mover", "Break 25 blocks"},
                                             {"Stone age", "Mine stone"},
                                             {"Black gold", "Find coal ore"},
                                             {"Iron country", "Find iron ore"},
                                             {"Diamonds!", "Find diamond ore"},
                                             {"A place to call home", "Place 25 blocks"},
                                             {"Master builder", "Place 100 blocks"},
                                             {"Hand crafted", "Craft your first item"},
                                             {"Woodworker", "Craft oak planks"},
                                             {"Let there be light", "Craft a lantern"},
                                             {"Through the looking glass", "Craft glass"},
                                             {"On the waterfront", "Enter the water"},
                                             {"Into the depths", "Travel below height 18"},
                                             {"Top of the world", "Reach height 65"},
                                             {"Night shift", "Survive until night"},
                                             {"A new day", "See the next sunrise"},
                                             {"Long way from home", "Walk 500 blocks"},
                                             {"Monster hunter", "Defeat a zombie"},
                                             {"Explorer", "Reach the far edge of the world"},
                                             {"Stockpiler", "Hold 128 blocks"},
                                             {"Making progress", "Earn 16 achievements"}};
int target[3], adjacent[3], has_target, selected = LOG, flying;
int meshes_dirty[CHUNKS][CHUNKS];
char data_dir[MAX_PATH], asset_dir[MAX_PATH], status_text[160];
float status_time;
static float vertical_velocity, attack_cooldown, hunger_clock;
static int grounded;
static size_t idx(int x, int y, int z) {
    return ((size_t)x * WORLD_Z + z) * WORLD_Y + y;
}
uint32_t hash_coords(int x, int y, int z, uint32_t seed) {
    uint32_t v = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)z * 2246822519u + seed;
    v = (v ^ (v >> 13)) * 1274126177u;
    return v ^ (v >> 16);
}
unsigned char block_at(int x, int y, int z) {
    if (x < 0 || z < 0 || x >= WORLD_X || z >= WORLD_Z || y < 0)
        return BEDROCK;
    if (y >= WORLD_Y)
        return AIR;
    return blocks[idx(x, y, z)];
}
int solid(int b) {
    return b != AIR && b != WATER && b != TORCH;
}
void set_block(int x, int y, int z, int b) {
    if (x < 0 || z < 0 || y < 0 || x >= WORLD_X || z >= WORLD_Z || y >= WORLD_Y)
        return;
    blocks[idx(x, y, z)] = (unsigned char)b;
    int cx = x / CHUNK, cz = z / CHUNK;
    meshes_dirty[cx][cz] = 1;
    if (x % CHUNK == 0 && cx > 0)
        meshes_dirty[cx - 1][cz] = 1;
    if (x % CHUNK == 15 && cx < CHUNKS - 1)
        meshes_dirty[cx + 1][cz] = 1;
    if (z % CHUNK == 0 && cz > 0)
        meshes_dirty[cx][cz - 1] = 1;
    if (z % CHUNK == 15 && cz < CHUNKS - 1)
        meshes_dirty[cx][cz + 1] = 1;
}
static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}
static float noise(float x, float z, int scale, uint32_t seed) {
    float fx = x / scale, fz = z / scale;
    int ix = (int)floorf(fx), iz = (int)floorf(fz);
    fx -= ix;
    fz -= iz;
    fx = fx * fx * (3 - 2 * fx);
    fz = fz * fz * (3 - 2 * fz);
    float a = (hash_coords(ix, 0, iz, seed) & 65535) / 65535.f;
    float b = (hash_coords(ix + 1, 0, iz, seed) & 65535) / 65535.f;
    float c = (hash_coords(ix, 0, iz + 1, seed) & 65535) / 65535.f;
    float d = (hash_coords(ix + 1, 0, iz + 1, seed) & 65535) / 65535.f;
    return lerp(lerp(a, b, fx), lerp(c, d, fx), fz);
}
static int surface(int x, int z) {
    for (int y = WORLD_Y - 2; y > 0; y--) {
        int b = block_at(x, y, z);
        if (solid(b) && b != LEAVES && b != LOG)
            return y;
    }
    return 32;
}
static Vec3 safe_spawn(void) {
    for (int ring = 0; ring < 100; ring++)
        for (int side = 0; side < 4; side++) {
            int x = WORLD_X / 2 + (side == 0   ? ring
                                   : side == 1 ? -ring
                                               : 0),
                z = WORLD_Z / 2 + (side == 2   ? ring
                                   : side == 3 ? -ring
                                               : 0);
            int y = surface(x, z);
            if (y > WATER_LEVEL && y < WORLD_Y - 4 && block_at(x, y + 1, z) == AIR &&
                block_at(x, y + 2, z) == AIR)
                return (Vec3){x + .5f, y + 1.01f, z + .5f};
        }
    int x = WORLD_X / 2, z = WORLD_Z / 2, y = max(WATER_LEVEL + 1, surface(x, z));
    set_block(x, y, z, GRASS);
    for (int i = 1; i < 4; i++)
        set_block(x, y + i, z, AIR);
    return (Vec3){x + .5f, y + 1.01f, z + .5f};
}
void notify(const char *s) {
    strncpy_s(status_text, sizeof(status_text), s, _TRUNCATE);
    status_time = 4;
}
void unlock(int id) {
    if (id < 0 || id >= ACH_COUNT || (world.achievements & (1u << id)))
        return;
    world.achievements |= 1u << id;
    char msg[160];
    snprintf(msg, sizeof(msg), "Achievement unlocked: %s", achievements[id].title);
    notify(msg);
    play_sound(2);
}
void generate_world(uint32_t seed, const char *name, int creative) {
    memset(&world, 0, sizeof(world));
    memset(blocks, 0, (size_t)WORLD_X * WORLD_Z * WORLD_Y);
    world.seed = seed;
    world.creative = creative;
    world.health = 20;
    world.hunger = 20;
    world.time = 1200;
    strncpy_s(world.name, sizeof(world.name), name, _TRUNCATE);
    for (int x = 0; x < WORLD_X; x++)
        for (int z = 0; z < WORLD_Z; z++) {
            float hills = noise((float)x, (float)z, 55, seed),
                  detail = noise((float)x, (float)z, 16, seed + 77);
            float biome = noise((float)x, (float)z, 70, seed + 51);
            int h = (int)(23 + hills * 31 + detail * 7);
            if (biome > .68f)
                h += 9;
            for (int y = 0; y <= h; y++) {
                int b = STONE;
                uint32_t r = hash_coords(x, y, z, seed);
                if (y == 0)
                    b = BEDROCK;
                else if (y == h)
                    b = (h < WATER_LEVEL + 2 || biome < .25f) ? SAND : (h > 57 ? SNOW : GRASS);
                else if (y > h - 4)
                    b = (h < WATER_LEVEL + 2 || biome < .25f) ? SAND : DIRT;
                else if (y < 16 && r % 211 == 0)
                    b = DIAMOND;
                else if (y < 37 && r % 93 == 0)
                    b = IRON;
                else if (r % 63 == 0)
                    b = COAL;
                if (y > 3 && y < h - 5) {
                    float cave = sinf(x * .16f + y * .23f) + sinf(z * .19f - y * .14f) +
                                 sinf((x + z) * .08f + y * .31f + (seed % 100));
                    if (cave > 2.15f)
                        b = AIR;
                }
                blocks[idx(x, y, z)] = (unsigned char)b;
            }
            for (int y = h + 1; y <= WATER_LEVEL; y++)
                blocks[idx(x, y, z)] = WATER;
        }
    for (int x = 3; x < WORLD_X - 3; x++)
        for (int z = 3; z < WORLD_Z - 3; z++) {
            int y = surface(x, z);
            if (block_at(x, y, z) != GRASS || hash_coords(x, 99, z, seed) % 147 != 0)
                continue;
            int height = 4 + (int)(hash_coords(x, 1, z, seed) % 3);
            for (int dy = 1; dy <= height; dy++)
                blocks[idx(x, y + dy, z)] = LOG;
            for (int dx = -2; dx <= 2; dx++)
                for (int dz = -2; dz <= 2; dz++)
                    for (int dy = height - 2; dy <= height + 1; dy++) {
                        if (abs(dx) + abs(dz) > 3 || y + dy >= WORLD_Y)
                            continue;
                        if (block_at(x + dx, y + dy, z + dz) == AIR)
                            blocks[idx(x + dx, y + dy, z + dz)] = LEAVES;
                    }
        }
    world.pos = safe_spawn();
    world.yaw = 0;
    for (int k = 0; k < BLOCK_COUNT; k++)
        world.inventory[k] = creative ? 999 : 0;
    if (!creative) {
        world.inventory[PLANKS] = 16;
        world.inventory[TORCH] = 8;
    }
    for (int i = 0; i < MOB_COUNT; i++) {
        int x = 8 + (int)(hash_coords(i, 0, 1, seed) % (WORLD_X - 16)),
            z = 8 + (int)(hash_coords(i, 0, 2, seed) % (WORLD_Z - 16));
        world.mobs[i] = (Mob){x + .5f, surface(x, z) + 1.f, z + .5f, 0, 0, 0, 0, i % 4, 10};
    }
    for (int x = 0; x < CHUNKS; x++)
        for (int z = 0; z < CHUNKS; z++)
            meshes_dirty[x][z] = 1;
    vertical_velocity = attack_cooldown = hunger_clock = 0;
    flying = creative;
    grounded = 0;
    unlock(0);
}
static int collides(float x, float y, float z) {
    for (int ix = (int)floorf(x - .29f); ix <= (int)floorf(x + .29f); ix++)
        for (int iz = (int)floorf(z - .29f); iz <= (int)floorf(z + .29f); iz++)
            for (int iy = (int)floorf(y + .01f); iy <= (int)floorf(y + 1.78f); iy++)
                if (solid(block_at(ix, iy, iz)))
                    return 1;
    return 0;
}
static void move_player(float dx, float dy, float dz) {
    float max = fmaxf(fabsf(dx), fmaxf(fabsf(dy), fabsf(dz)));
    int steps = (int)(max / .15f) + 1;
    dx /= steps;
    dy /= steps;
    dz /= steps;
    for (int i = 0; i < steps; i++) {
        if (!collides(world.pos.x + dx, world.pos.y, world.pos.z))
            world.pos.x += dx;
        if (!collides(world.pos.x, world.pos.y, world.pos.z + dz))
            world.pos.z += dz;
        if (!collides(world.pos.x, world.pos.y + dy, world.pos.z))
            world.pos.y += dy;
        else {
            if (dy < 0)
                grounded = 1;
            vertical_velocity = 0;
        }
    }
    world.pos.y = fminf(world.pos.y, WORLD_Y - 2.f);
}
void update_target(void) {
    float yaw = world.yaw * (float)(PI / 180), pitch = world.pitch * (float)(PI / 180);
    Vec3 p = {world.pos.x, world.pos.y + 1.62f, world.pos.z},
         dir = {sinf(yaw) * cosf(pitch), -sinf(pitch), -cosf(yaw) * cosf(pitch)};
    has_target = 0;
    int prev[3] = {(int)floorf(p.x), (int)floorf(p.y), (int)floorf(p.z)};
    for (float t = 0; t < 6; t += .025f) {
        int q[3] = {(int)floorf(p.x + dir.x * t), (int)floorf(p.y + dir.y * t), (int)floorf(p.z + dir.z * t)};
        int b = block_at(q[0], q[1], q[2]);
        if (b != AIR && b != WATER) {
            memcpy(target, q, sizeof(target));
            memcpy(adjacent, prev, sizeof(adjacent));
            has_target = 1;
            return;
        }
        memcpy(prev, q, sizeof(prev));
    }
}
void update_game(float dt) {
    attack_cooldown = fmaxf(0, attack_cooldown - dt);
    world.time += dt * 20;
    float yaw = world.yaw * (float)(PI / 180), fx = sinf(yaw), fz = -cosf(yaw), sx = cosf(yaw),
          sz = sinf(yaw);
    float f = (float)(keys['W'] - keys['S']), s = (float)(keys['D'] - keys['A']);
    float len = sqrtf(f * f + s * s);
    if (len > 0) {
        f /= len;
        s /= len;
    }
    int water = block_at((int)world.pos.x, (int)(world.pos.y + .7f), (int)world.pos.z) == WATER;
    float speed = flying ? (keys[VK_CONTROL] ? 22 : 10) : (keys[VK_CONTROL] ? 6.7f : 4.3f);
    if (keys[VK_SHIFT] && !flying)
        speed = 1.8f;
    if (water && !flying)
        speed = 2.6f;
    float ox = world.pos.x, oz = world.pos.z;
    if (flying) {
        vertical_velocity = 0;
        move_player((fx * f + sx * s) * speed * dt, (keys[VK_SPACE] - keys[VK_SHIFT]) * speed * dt,
                    (fz * f + sz * s) * speed * dt);
    } else {
        if (keys[VK_SPACE] && (grounded || water)) {
            vertical_velocity = water ? 3.7f : 8.2f;
            grounded = 0;
        }
        vertical_velocity -= (water ? 6.f : 24.f) * dt;
        if (water)
            vertical_velocity = fmaxf(vertical_velocity, -2.f);
        grounded = 0;
        move_player((fx * f + sx * s) * speed * dt, vertical_velocity * dt, (fz * f + sz * s) * speed * dt);
    }
    float distance = sqrtf((world.pos.x - ox) * (world.pos.x - ox) + (world.pos.z - oz) * (world.pos.z - oz));
    world.walk_distance += distance;
    world.walked = (uint32_t)world.walk_distance;
    if (world.walked >= 50)
        unlock(1);
    if (world.walked >= 500)
        unlock(19);
    if (water)
        unlock(14);
    if (world.pos.y < 18)
        unlock(15);
    if (world.pos.y >= 65)
        unlock(16);
    if ((int)world.time % 24000 > 13000)
        unlock(17);
    if (world.time > 24000)
        unlock(18);
    if (world.pos.x < 15 || world.pos.x > WORLD_X - 15 || world.pos.z < 15 || world.pos.z > WORLD_Z - 15)
        unlock(21);
    if (world.pos.y < 1) {
        world.pos.y = (float)surface((int)world.pos.x, (int)world.pos.z) + 2;
        world.health = 20;
    }
    hunger_clock += dt;
    if (hunger_clock > 30) {
        hunger_clock = 0;
        if (!world.creative)
            world.hunger = fmaxf(0, world.hunger - .5f);
        if (world.hunger > 4)
            world.health = fminf(20, world.health + 1);
    }
    for (int i = 0; i < MOB_COUNT; i++) {
        Mob *m = &world.mobs[i];
        if (m->hp <= 0)
            continue;
        m->think -= dt;
        float dx = world.pos.x - m->x, dz = world.pos.z - m->z, d = sqrtf(dx * dx + dz * dz);
        int hostile = m->kind == 3 && ((int)world.time % 24000 > 13000);
        if (m->think <= 0) {
            m->yaw = (hash_coords(i, (int)world.time, 0, world.seed) % 6283) / 1000.f;
            m->think = 2 + (float)(i % 3);
        }
        float vx = sinf(m->yaw), vz = cosf(m->yaw);
        if (hostile && d < 22 && d > .01f) {
            vx = dx / d;
            vz = dz / d;
            m->yaw = atan2f(vx, vz);
        }
        float nx = m->x + vx * dt * (hostile ? 2.1f : 1.1f), nz = m->z + vz * dt * (hostile ? 2.1f : 1.1f);
        if (d > 72)
            continue;
        int top = surface((int)nx, (int)nz);
        if (nx > 2 && nz > 2 && nx < WORLD_X - 2 && nz < WORLD_Z - 2 && abs(top + 1 - (int)m->y) <= 1 &&
            top >= WATER_LEVEL && !solid(block_at((int)nx, top + 1, (int)nz))) {
            m->x = nx;
            m->z = nz;
            m->y = (float)top + 1.f;
        } else
            m->think = 0;
        if (hostile && d < 1.2f && !world.creative && attack_cooldown <= 0) {
            world.health -= 2;
            attack_cooldown = 1;
            play_sound(1);
        }
    }
    if (world.health <= 0) {
        world.health = 20;
        world.hunger = 20;
        world.pos = safe_spawn();
        notify("You respawned. Your inventory was kept.");
    }
    uint32_t count = 0, total = 0;
    for (int i = 0; i < ACH_COUNT; i++)
        if (world.achievements & (1u << i))
            count++;
    if (count >= 16)
        unlock(23);
    for (int i = 1; i < BLOCK_COUNT; i++)
        total += world.inventory[i];
    if (total >= 128 && !world.creative)
        unlock(22);
    update_target();
}
void interact(int place) {
    if (!place) {
        float yaw = world.yaw * (float)(PI / 180), pitch = world.pitch * (float)(PI / 180);
        Vec3 dir = {sinf(yaw) * cosf(pitch), -sinf(pitch), -cosf(yaw) * cosf(pitch)};
        float blockdist = has_target ? sqrtf(powf(target[0] + .5f - world.pos.x, 2) +
                                             powf(target[1] + .5f - world.pos.y - 1.62f, 2) +
                                             powf(target[2] + .5f - world.pos.z, 2))
                                     : 6;
        for (int i = 0; i < MOB_COUNT; i++) {
            Mob *m = &world.mobs[i];
            if (m->hp <= 0)
                continue;
            float dx = m->x - world.pos.x, dy = m->y + .7f - world.pos.y - 1.62f, dz = m->z - world.pos.z;
            float t = dx * dir.x + dy * dir.y + dz * dir.z;
            if (t > 0 && t < 4 && t < blockdist && dx * dx + dy * dy + dz * dz - t * t < .65f) {
                m->hp -= 5;
                play_sound(1);
                if (m->hp <= 0) {
                    world.killed++;
                    world.hunger = fminf(20, world.hunger + 5);
                    world.health = fminf(20, world.health + 2);
                    if (m->kind == 3)
                        unlock(20);
                    notify("Mob defeated. Food restored.");
                }
                return;
            }
        }
    }
    if (!has_target)
        return;
    if (!place) {
        int b = block_at(target[0], target[1], target[2]);
        if (b == BEDROCK)
            return;
        set_block(target[0], target[1], target[2], AIR);
        world.inventory[b]++;
        world.broken++;
        if (b == LOG)
            unlock(2);
        if (world.broken >= 25)
            unlock(3);
        if (b == STONE)
            unlock(4);
        if (b == COAL)
            unlock(5);
        if (b == IRON)
            unlock(6);
        if (b == DIAMOND)
            unlock(7);
        play_sound(0);
    } else {
        int x = adjacent[0], y = adjacent[1], z = adjacent[2];
        if (x < 0 || z < 0 || y < 1 || x >= WORLD_X || z >= WORLD_Z || y >= WORLD_Y)
            return;
        if (!world.creative && !world.inventory[selected]) {
            notify("No blocks left. Mine or craft more, or choose another slot.");
            return;
        }
        int old = block_at(x, y, z);
        set_block(x, y, z, selected);
        if (collides(world.pos.x, world.pos.y, world.pos.z)) {
            set_block(x, y, z, old);
            return;
        }
        if (!world.creative)
            world.inventory[selected]--;
        world.placed++;
        if (world.placed >= 25)
            unlock(8);
        if (world.placed >= 100)
            unlock(9);
        play_sound(0);
    }
}
int craft_item(int recipe) {
    int output = PLANKS, a = LOG, ac = 1, b = AIR, bc = 0, amount = 4;
    if (recipe == 1) {
        output = BRICK;
        a = STONE;
        ac = 4;
        amount = 4;
    }
    if (recipe == 2) {
        output = GLASS;
        a = SAND;
        ac = 4;
        b = COAL;
        bc = 1;
        amount = 4;
    }
    if (recipe == 3) {
        output = TORCH;
        a = PLANKS;
        ac = 1;
        b = COAL;
        bc = 1;
        amount = 8;
    }
    if (!world.creative && (world.inventory[a] < (uint32_t)ac || world.inventory[b] < (uint32_t)bc)) {
        notify("You need more ingredients for this recipe.");
        return 0;
    }
    if (!world.creative) {
        world.inventory[a] -= ac;
        world.inventory[b] -= bc;
    }
    world.inventory[output] += amount;
    world.crafted++;
    unlock(10);
    if (output == PLANKS)
        unlock(11);
    if (output == TORCH)
        unlock(12);
    if (output == GLASS)
        unlock(13);
    notify("Crafted successfully. Select it from your inventory.");
    return 1;
}
typedef struct {
    char magic[8];
    uint32_t version, state_size, block_size, checksum;
} SaveHeader;
static uint32_t checksum_bytes(const unsigned char *p, size_t n, uint32_t h) {
    for (size_t i = 0; i < n; i++)
        h = (h ^ p[i]) * 16777619u;
    return h;
}
static void save_path(char *p, size_t n, int slot, const char *suffix) {
    snprintf(p, n, "%s\\world-%d%s", data_dir, slot, suffix);
}
int save_world(int slot) {
    char path[MAX_PATH], tmp[MAX_PATH];
    save_path(path, sizeof(path), slot, ".pxw");
    save_path(tmp, sizeof(tmp), slot, ".tmp");
    size_t n = (size_t)WORLD_X * WORLD_Z * WORLD_Y;
    SaveHeader h = {{'P', 'A', 'R', 'A', 'L', 'L', 'A', 'X'}, SAVE_VERSION, sizeof(world), (uint32_t)n, 0};
    h.checksum = checksum_bytes((unsigned char *)&world, sizeof(world), 2166136261u);
    h.checksum = checksum_bytes(blocks, n, h.checksum);
    FILE *f = NULL;
    if (fopen_s(&f, tmp, "wb") || !f) {
        notify("Save failed: could not open the world file.");
        return 0;
    }
    int ok = fwrite(&h, sizeof(h), 1, f) == 1 && fwrite(&world, sizeof(world), 1, f) == 1 &&
             fwrite(blocks, 1, n, f) == n;
    if (fflush(f) != 0)
        ok = 0;
    if (fclose(f) != 0)
        ok = 0;
    if (ok)
        ok = MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    if (!ok)
        notify("Save failed. Your previous save was kept.");
    return ok;
}
static int read_save(int slot, WorldState *state, unsigned char *dest) {
    char path[MAX_PATH];
    save_path(path, sizeof(path), slot, ".pxw");
    FILE *f = NULL;
    if (fopen_s(&f, path, "rb") || !f)
        return 0;
    SaveHeader h;
    int ok = fread(&h, sizeof(h), 1, f) == 1 && memcmp(h.magic, "PARALLAX", 8) == 0 &&
             h.version == SAVE_VERSION && h.state_size == sizeof(WorldState) &&
             h.block_size == WORLD_X * WORLD_Z * WORLD_Y;
    if (ok)
        ok = fread(state, sizeof(*state), 1, f) == 1;
    if (ok && dest) {
        ok = fread(dest, 1, h.block_size, f) == h.block_size;
        if (ok) {
            uint32_t sum = checksum_bytes((unsigned char *)state, sizeof(*state), 2166136261u);
            sum = checksum_bytes(dest, h.block_size, sum);
            ok = sum == h.checksum;
        }
        if (ok)
            for (uint32_t i = 0; i < h.block_size; i++)
                if (dest[i] >= BLOCK_COUNT) {
                    ok = 0;
                    break;
                }
        if (ok)
            ok = isfinite(state->pos.x) && isfinite(state->pos.y) && isfinite(state->pos.z) &&
                 isfinite(state->yaw) && isfinite(state->pitch) && isfinite(state->time) &&
                 isfinite(state->health) && state->pos.x >= 0 && state->pos.x < WORLD_X &&
                 state->pos.z >= 0 && state->pos.z < WORLD_Z && state->pos.y >= 0 && state->pos.y < WORLD_Y;
    }
    state->name[47] = 0;
    fclose(f);
    return ok;
}
int save_info(int slot, WorldState *info) {
    memset(info, 0, sizeof(*info));
    return read_save(slot, info, NULL);
}
int load_world(int slot) {
    unsigned char *tmp = (unsigned char *)malloc((size_t)WORLD_X * WORLD_Z * WORLD_Y);
    WorldState state;
    memset(&state, 0, sizeof(state));
    if (!tmp)
        return 0;
    int ok = read_save(slot, &state, tmp);
    if (ok) {
        world = state;
        memcpy(blocks, tmp, (size_t)WORLD_X * WORLD_Z * WORLD_Y);
        for (int x = 0; x < CHUNKS; x++)
            for (int z = 0; z < CHUNKS; z++)
                meshes_dirty[x][z] = 1;
        flying = world.creative;
        vertical_velocity = 0;
        unlock(0);
    } else
        notify("Cannot load this world: unsupported or damaged save.");
    free(tmp);
    return ok;
}
int self_test(void) {
    int failures = 0;
    FILE *report = NULL;
    fopen_s(&report, "self-test-results.txt", "w");
#define CHECK_TEST(expr, label)                                                                              \
    do {                                                                                                     \
        int passed = (expr);                                                                                 \
        if (!passed)                                                                                         \
            failures++;                                                                                      \
        if (report)                                                                                          \
            fprintf(report, "%s: %s\n", label, passed ? "PASS" : "FAIL");                                    \
    } while (0)
    char original[MAX_PATH];
    strcpy_s(original, sizeof(original), data_dir);
    snprintf(data_dir, sizeof(data_dir), "%s\\selftest-%lu", original, GetCurrentProcessId());
    CreateDirectoryA(data_dir, NULL);
    generate_world(12345, "Test world", 0);
    size_t n = (size_t)WORLD_X * WORLD_Z * WORLD_Y;
    uint32_t first = checksum_bytes(blocks, n, 2166136261u);
    generate_world(12345, "Test world", 0);
    CHECK_TEST(first == checksum_bytes(blocks, n, 2166136261u), "Deterministic generation");
    generate_world(999, "Other", 0);
    CHECK_TEST(first != checksum_bytes(blocks, n, 2166136261u), "Distinct seeds");
    CHECK_TEST(!collides(world.pos.x, world.pos.y, world.pos.z), "Safe spawn");
    for (int x = 120; x <= 136; x++)
        for (int z = 120; z <= 136; z++)
            for (int y = 48; y <= 56; y++)
                set_block(x, y, z, y == 48 ? STONE : AIR);
    world.pos = (Vec3){128.5f, 49.01f, 128.5f};
    move_player(2, 0, 0);
    CHECK_TEST(world.pos.x > 130.4f, "Movement across free space");
    set_block(132, 49, 128, STONE);
    set_block(132, 50, 128, STONE);
    move_player(5, 0, 0);
    CHECK_TEST(world.pos.x < 131.72f, "Collision prevents walking through walls");
    move_player(0, -4, 0);
    CHECK_TEST(world.pos.y >= 49 && grounded, "Ground collision");
    world.pos.y = 94;
    move_player(0, 100, 0);
    CHECK_TEST(world.pos.y <= 94, "Flight height remains saveable");
    world.pos.y = 49.01f;
    uint32_t old_planks = world.inventory[PLANKS];
    CHECK_TEST(!craft_item(2) && world.inventory[PLANKS] == old_planks, "Missing ingredients rejected");
    set_block(1, 10, 1, DIAMOND);
    world.inventory[LOG] = 3;
    craft_item(0);
    CHECK_TEST(world.inventory[LOG] == 2 && world.inventory[PLANKS] == 20, "Crafting accounting");
    uint32_t before = checksum_bytes(blocks, n, 2166136261u);
    CHECK_TEST(save_world(99), "Save creation");
    set_block(1, 10, 1, AIR);
    CHECK_TEST(load_world(99) && before == checksum_bytes(blocks, n, 2166136261u) &&
                   block_at(1, 10, 1) == DIAMOND,
               "Save round trip");
    char p[MAX_PATH];
    save_path(p, sizeof(p), 99, ".pxw");
    FILE *f = NULL;
    fopen_s(&f, p, "r+b");
    if (f) {
        fseek(f, -1, SEEK_END);
        fputc(255, f);
        fclose(f);
    }
    CHECK_TEST(!load_world(99), "Corruption rejected");
    CHECK_TEST(block_at(1, 10, 1) == DIAMOND, "Failed load preserves active world");
    DeleteFileA(p);
    RemoveDirectoryA(data_dir);
    strcpy_s(data_dir, sizeof(data_dir), original);
    if (report) {
        fprintf(report, "Parallax native C self-test: %s\nFailures: %d\n", failures ? "FAIL" : "PASS",
                failures);
        fclose(report);
    }
    return failures;
#undef CHECK_TEST
}
