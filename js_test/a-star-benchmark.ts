import * as Pathfinder from 'pathfinding';
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

function getInteractableOrDenseObject(x: number, y: number) {
    return getXYObjectFromHash(x, y, denseHash) || getXYObjectFromHash(x, y, interactableHash);
}

function checkIfDenseObject(x: number, y: number): boolean {
    const object = getInteractableOrDenseObject(x, y);
    return object && object.density && object.type !== 'Door';
}

map = formatMapData(cloneDeep(require('../assets/maps/antania/DedlaenMaze.json')));

initFov();

const x = 9;
const y = 9;
const playerx = 1;
const playery = 1;
const start = performance.now();
for(let i = 0; i < 10000; i++) {
    const denseTiles = map.layers[MapLayer.Walls].data;

    const grid = new Pathfinder.Grid(19, 19);

    for(let gx = -9; gx <= 9; gx++) {
        for(let gy = -9; gy <= 9; gy++) {

            const nextTileLoc = ((playery + gy) * map.width) + (playerx + gx);
            const nextTile = denseTiles[nextTileLoc];

            // dense tiles get set to false
            if(nextTile !== 0) {
                grid.setWalkableAt(gx + 9, gy + 9, false);

                // non-dense tiles get checked for objects
            } else {
                if(checkIfDenseObject(playerx + gx, playery + gy)) {
                    grid.setWalkableAt(gx + 9, gy + 9, false);
                }

            }

        }
    }

    grid.setWalkableAt(x + 9, y + 9, true);

    const astar = new Pathfinder.AStarFinder({
        diagonalMovement: Pathfinder.DiagonalMovement.Always,
        // dontCrossCorners: false
    });

    const finalPath = astar.findPath(4, 4, 4 + x, 4 + y, grid);


}
let end = performance.now() - start;

console.log(`time: ${end}`);
