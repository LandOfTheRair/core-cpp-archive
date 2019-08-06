START TRANSACTION;

INSERT INTO maps (map_name) VALUES ('test_map');
INSERT INTO map_tiles (map_id, tile_id, x, y, z) VALUES (1, 0, 0, 0, 0), (1, 0, 0, 1, 0), (1, 0, 1, 0, 0), (1, 0, 1, 1, 0);

INSERT INTO schema_information(file_name, date) VALUES ('test_data.sql', CURRENT_TIMESTAMP);

COMMIT;