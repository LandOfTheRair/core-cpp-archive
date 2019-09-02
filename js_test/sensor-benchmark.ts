import { CensorSensor } from 'censor-sensor';
declare function require(name:string);
const { PerformanceObserver, performance } = require('perf_hooks');
//import { performance } from 'perf_hooks';

const censor = new CensorSensor();

let start = performance.now();
for(let i = 0; i < 10000000; i++) {
    censor.isProfane("this is bollocks");
}
let end = performance.now() - start;

console.log(`time: ${end}`);