START TRANSACTION;

CREATE TABLE users (
    id BIGSERIAL PRIMARY KEY,
    username CITEXT NOT NULL,
    password VARCHAR(100) NOT NULL,
    email CITEXT NOT NULL,
    login_attempts SMALLINT NOT NULL DEFAULT 0,
    verification_code text DEFAULT NULL,
    no_of_players SMALLINT NOT NULL DEFAULT 0,
    admin SMALLINT NOT NULL DEFAULT 0
);

CREATE TABLE licenses (
    id BIGSERIAL PRIMARY KEY,
    license_name text NOT NULL,
    author text NOT NULL,
    license text NOT NULL
);

CREATE TABLE banned_users (
    id BIGSERIAL PRIMARY KEY,
    ip text NULL,
    user_id BIGINT NULL,
    until BIGINT NULL
);

CREATE TABLE stats (
    id SERIAL PRIMARY KEY,
    stat_name CITEXT NOT NULL
);

CREATE TABLE locations (
    id BIGSERIAL PRIMARY KEY,
    MAP_NAME CITEXT NOT NULL,
    x INT NOT NULL,
    y INT NOT NULL
);

CREATE TABLE players (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL,
    location_id BIGINT NOT NULL,
    player_name CITEXT NOT NULL
);

CREATE TABLE player_stats (
    id BIGSERIAL PRIMARY KEY,
    player_id BIGINT NOT NULL,
    stat_id INT NOT NULL,
    is_growth BOOLEAN NOT NULL,
    static_value BIGINT NOT NULL,
    dice INT NULL,
    die_face INT NULL
);

CREATE TABLE items (
    id BIGSERIAL PRIMARY KEY,
    player_id BIGINT NULL,
    npc_id INT NULL,
    location_id BIGINT NULL,
    item_name CITEXT NOT NULL
);

CREATE TABLE item_stats (
    id BIGSERIAL PRIMARY KEY,
    item_id BIGINT NOT NULL,
    stat_id INT NOT NULL,
    is_growth BOOLEAN NOT NULL,
    static_value BIGINT NOT NULL,
    dice INT NULL,
    die_face INT NULL
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
    value text not null
);

CREATE TABLE schema_information (
    file_name text NOT NULL,
    date TIMESTAMPTZ NOT NULL
);

ALTER TABLE users ADD CONSTRAINT "users_username_unique" UNIQUE (username);
ALTER TABLE banned_users ADD CONSTRAINT "banned_users_user_id_fkey" FOREIGN KEY (user_id) REFERENCES users(id);
ALTER TABLE players ADD CONSTRAINT "players_locations_id_fkey" FOREIGN KEY (location_id) REFERENCES locations(id);
ALTER TABLE players ADD CONSTRAINT "players_users_id_fkey" FOREIGN KEY (user_id) REFERENCES users(id);
ALTER TABLE players ADD CONSTRAINT "players_name_unique" UNIQUE (player_name);
ALTER TABLE player_stats ADD CONSTRAINT "player_stats_players_id_fkey" FOREIGN KEY (player_id) REFERENCES players(id);
ALTER TABLE player_stats ADD CONSTRAINT "player_stats_stats_id_fkey" FOREIGN KEY (stat_id) REFERENCES stats(id);
ALTER TABLE player_stats ADD CONSTRAINT "player_stats_die_ck" CHECK ((dice IS NOT NULL AND die_face IS NOT NULL) OR (dice IS NULL AND die_face IS NULL));
ALTER TABLE items ADD CONSTRAINT "items_players_id_fkey" FOREIGN KEY (player_id) REFERENCES players(id);
ALTER TABLE items ADD CONSTRAINT "items_npcs_id_fkey" FOREIGN KEY (npc_id) REFERENCES npcs(id);
ALTER TABLE items ADD CONSTRAINT "items_locations_id_fkey" FOREIGN KEY (location_id) REFERENCES locations(id);
ALTER TABLE item_stats ADD CONSTRAINT "item_stats_items_id_fkey" FOREIGN KEY (item_id) REFERENCES items(id);
ALTER TABLE item_stats ADD CONSTRAINT "item_stats_stats_id_fkey" FOREIGN KEY (stat_id) REFERENCES stats(id);
ALTER TABLE item_stats ADD CONSTRAINT "item_stats_die_ck" CHECK ((dice IS NOT NULL AND die_face IS NOT NULL) OR (dice IS NULL AND die_face IS NULL));
ALTER TABLE npcs ADD CONSTRAINT "npcs_location_id_fkey" FOREIGN KEY (location_id) REFERENCES locations(id);
ALTER TABLE npc_stats ADD CONSTRAINT "npc_stats_items_id_fkey" FOREIGN KEY (npc_id) REFERENCES npcs(id);
ALTER TABLE npc_stats ADD CONSTRAINT "npc_stats_stats_id_fkey" FOREIGN KEY (stat_id) REFERENCES stats(id);
ALTER TABLE npc_stats ADD CONSTRAINT "npc_stats_die_ck" CHECK ((dice IS NOT NULL AND die_face IS NOT NULL) OR (dice IS NULL AND die_face IS NULL));
ALTER TABLE objects ADD CONSTRAINT "objects_locations_id_fkey" FOREIGN KEY (location_id) REFERENCES locations(id);
ALTER TABLE settings ADD CONSTRAINT "settings_name_unique" UNIQUE (setting_name);
ALTER TABLE schema_information ADD CONSTRAINT "schema_information_name_unique" UNIQUE (file_name);

INSERT INTO schema_information(file_name, date) VALUES ('init.sql', CURRENT_TIMESTAMP);

COMMIT;