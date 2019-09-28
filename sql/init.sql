START TRANSACTION;

CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    username CITEXT NOT NULL,
    password VARCHAR(100) NOT NULL,
    email CITEXT NOT NULL,
    login_attempts SMALLINT NOT NULL DEFAULT 0,
    verification_code TEXT DEFAULT NULL,
    max_characters SMALLINT NOT NULL DEFAULT 0,
    subscription_tier SMALLINT NOT NULL DEFAULT 0,
    is_tester SMALLINT NOT NULL DEFAULT 0,
    is_game_master SMALLINT NOT NULL DEFAULT 0,
    trial_ends SMALLINT NOT NULL DEFAULT 0,
    has_done_trial SMALLINT NOT NULL DEFAULT 0,
    discord_tag TEXT DEFAULT NULL,
    discord_online SMALLINT NOT NULL DEFAULT 0
);

CREATE TABLE banned_users (
    id BIGSERIAL PRIMARY KEY,
    ip TEXT NULL,
    user_id BIGINT NULL,
    until BIGINT NULL
);

CREATE TABLE silver_purchase_possibilities (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NULL,
    name TEXT NOT NULL,
    count INT NOT NULL DEFAULT 0
);

CREATE TABLE licenses (
    id BIGSERIAL PRIMARY KEY,
    license_name TEXT NOT NULL,
    author TEXT NOT NULL,
    license TEXT NOT NULL,
    software_name TEXT NOT NULL
);

CREATE TABLE locations (
    id BIGSERIAL PRIMARY KEY,
    MAP_NAME CITEXT NOT NULL,
    x INT NOT NULL,
    y INT NOT NULL
);

CREATE TABLE characters (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    location_id BIGINT NOT NULL,
    slot INT NOT NULL,
    level INT NOT NULL,
    gold INT NOT NULL,
    character_name CITEXT NOT NULL,
    allegiance CITEXT NOT NULL,
    gender CITEXT NOT NULL,
    alignment CITEXT NOT NULL,
    class CITEXT NOT NULL
);

CREATE TABLE character_stats (
    id BIGSERIAL PRIMARY KEY,
    character_id BIGINT NOT NULL,
    stat_name TEXT NOT NULL,
    value BIGINT NOT NULL
);

CREATE TABLE character_skills (
    id BIGSERIAL PRIMARY KEY,
    character_id BIGINT NOT NULL,
    skill_name TEXT NOT NULL,
    value BIGINT NOT NULL
);

CREATE TABLE items (
    id BIGSERIAL PRIMARY KEY,
    character_id BIGINT NULL,
    npc_id INT NULL,
    location_id BIGINT NULL,
    item_name CITEXT NOT NULL,
    item_slot CITEXT NOT NULL
);

CREATE TABLE item_stats (
    id BIGSERIAL PRIMARY KEY,
    item_id BIGINT NOT NULL,
    stat_name CITEXT NOT NULL,
    value BIGINT NOT NULL
);

CREATE TABLE npcs (
    id SERIAL PRIMARY KEY,
    location_id BIGINT NOT NULL,
    npc_name CITEXT NOT NULL
);

CREATE TABLE npc_stats (
    id BIGSERIAL PRIMARY KEY,
    npc_id INT NOT NULL,
    stat_id INT NOT NULL,
    is_growth BOOLEAN NOT NULL,
    static_value BIGINT NOT NULL,
    dice INT NULL,
    die_face INT NULL
);

CREATE TABLE objects (
    id BIGSERIAL PRIMARY KEY,
    location_id BIGINT NOT NULL,
    object_name CITEXT NOT NULL
);

CREATE TABLE settings (
    setting_name CITEXT NOT NULL,
    value TEXT NOT NULL
);

CREATE TABLE schema_information (
    file_name TEXT NOT NULL,
    date TIMESTAMPTZ NOT NULL
);

ALTER TABLE users ADD CONSTRAINT "users_username_unique" UNIQUE (username);
ALTER TABLE banned_users ADD CONSTRAINT "banned_users_user_id_fkey" FOREIGN KEY (user_id) REFERENCES users(id);
ALTER TABLE silver_purchase_possibilities ADD CONSTRAINT "silver_purchase_possibilities_user_id_fkey" FOREIGN KEY (user_id) REFERENCES users(id);
ALTER TABLE characters ADD CONSTRAINT "characters_locations_id_fkey" FOREIGN KEY (location_id) REFERENCES locations(id);
ALTER TABLE characters ADD CONSTRAINT "characters_users_id_fkey" FOREIGN KEY (user_id) REFERENCES users(id);
ALTER TABLE characters ADD CONSTRAINT "characters_slot_unique" UNIQUE (user_id, slot);
ALTER TABLE character_stats ADD CONSTRAINT "character_stats_characters_id_fkey" FOREIGN KEY (character_id) REFERENCES characters(id);
ALTER TABLE items ADD CONSTRAINT "items_characters_id_fkey" FOREIGN KEY (character_id) REFERENCES characters(id);
ALTER TABLE items ADD CONSTRAINT "items_npcs_id_fkey" FOREIGN KEY (npc_id) REFERENCES npcs(id);
ALTER TABLE items ADD CONSTRAINT "items_locations_id_fkey" FOREIGN KEY (location_id) REFERENCES locations(id);
ALTER TABLE item_stats ADD CONSTRAINT "item_stats_items_id_fkey" FOREIGN KEY (item_id) REFERENCES items(id);
ALTER TABLE npcs ADD CONSTRAINT "npcs_location_id_fkey" FOREIGN KEY (location_id) REFERENCES locations(id);
ALTER TABLE npc_stats ADD CONSTRAINT "npc_stats_items_id_fkey" FOREIGN KEY (npc_id) REFERENCES npcs(id);
ALTER TABLE objects ADD CONSTRAINT "objects_locations_id_fkey" FOREIGN KEY (location_id) REFERENCES locations(id);
ALTER TABLE settings ADD CONSTRAINT "settings_name_unique" UNIQUE (setting_name);
ALTER TABLE schema_information ADD CONSTRAINT "schema_information_name_unique" UNIQUE (file_name);

INSERT INTO schema_information(file_name, date) VALUES ('init.sql', CURRENT_TIMESTAMP);

COMMIT;
