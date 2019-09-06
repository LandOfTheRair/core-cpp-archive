import * as Mrpas from 'mrpas';
import { set, get, cloneDeep } from 'lodash';
declare function require(name:string);
const { performance } = require('perf_hooks');

const MapLayer = {
    Terrain: 0,
    Floors: 1,
    Fluids: 2,
    Foliage: 3,
    Walls: 4,
    Decor: 5,
    DenseDecor: 6,
    OpaqueDecor: 7,
    Interactables: 8,
    NPCs: 9,
    Spawners: 10,
    RegionDescriptions: 11,
    BackgroundMusic: 12,
    Succorport: 13,
    SpawnerZones: 14
};

enum TilesWithNoFOVUpdate {
    Empty = 0,
    Air = 2386
}

let map: any;
let fov: any;
let interactableHash: any = {};
let denseHash: any = {};
let opaqueHash: any = {};

class ICharacter {
    x: number;
    y: number;
    fov: any;
}

function getXYObjectFromHash(x: number, y: number, hash) {
    return get(hash, [x, y + 1], null);
}

function addXYObjectToHash(obj, hash) {
    const setX = obj.x / 64;
    const setY = (obj.y / 64);

    hash[setX] = hash[setX] || {};
    hash[setX][setY] = obj;
}

function initFov() {
    const denseLayer = map.layers[MapLayer.Walls].data;
    const opaqueObjects = map.layers[MapLayer.OpaqueDecor].objects;
    opaqueObjects.forEach(obj => {
        obj.opacity = 1;

        addXYObjectToHash(obj, opaqueHash);
    });

    const denseObjects = map.layers[MapLayer.DenseDecor].objects;
    denseObjects.forEach(obj => {
        obj.density = 1;

        addXYObjectToHash(obj, denseHash);
    });

    const interactables = map.layers[MapLayer.Interactables].objects;
    interactables.forEach(obj => {
        if(obj.type === 'Door') {
            obj.opacity = 1;
            obj.density = 1;
        }

        addXYObjectToHash(obj, interactableHash);
    });

    fov = new Mrpas(map.width, map.height, (x, y) => {
        const tile = denseLayer[(y * map.width) + x];
        if(tile === TilesWithNoFOVUpdate.Empty || tile === TilesWithNoFOVUpdate.Air) {
            const object = getXYObjectFromHash(x, y, interactableHash) || getXYObjectFromHash(x, y, opaqueHash);
            return !object || (object && !object.opacity);
        }
        return false;
    });
}

function calculateFOV(player: ICharacter): void {
    if(!player) return;

    const affected = {};

    const dist = 4;

    fov.compute(player.x, player.y, dist, (x, y) => {
        return affected[x - player.x] && affected[x - player.x][y - player.y];
    }, (x, y) => {
        affected[x - player.x] = affected[x - player.x] || {};
        affected[x - player.x][y - player.y] = true;
    });

    player.fov = affected;
}

function formatMapData(mapData) {
    const layer = mapData.layers[MapLayer.SpawnerZones];

    let currentSpawnId = 1;

    if(!layer.objects.length) {
        layer.objects.push({
            x: 0,
            y: 0,
            width: mapData.width * 64,
            height: mapData.height * 64
        });
    }

    layer.objects.forEach(obj => {
        const spawnRegionId = get(obj, 'properties.spawnerRegionId');
        if(!spawnRegionId) {
            set(obj, 'properties.spawnerRegionId', currentSpawnId++);
        }
    });

    return mapData;
}

map = formatMapData(cloneDeep(require('../assets/maps/antania/DedlaenMaze.json')));

initFov();

let player = new ICharacter();
let start = performance.now();
for(let i = 0; i < 1000000; i++) {
    calculateFOV(player);
}
let end = performance.now() - start;

console.log(`time: ${end}`);
