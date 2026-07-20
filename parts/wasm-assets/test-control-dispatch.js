const path = require("node:path");

const modulePath = path.resolve(process.argv[2] || "skred_api.js");
let finished = false;

function fail(error) {
  if (finished) return;
  finished = true;
  console.error(error && error.stack ? error.stack : error);
  process.exit(1);
}

process.on("uncaughtException", fail);
process.on("unhandledRejection", fail);

const Module = require(modulePath);
Module.onRuntimeInitialized = () => {
  try {
    Module.ccall("skred_command", "number", ["string"], ["/mv 0 0"]);
    if (Module._skred_control_dispatch_running() !== 1)
      throw new Error("/mv did not start the control dispatcher");
    setTimeout(() => {
      try {
        Module._skred_control_dispatch_stop();
        if (Module._skred_control_dispatch_running() !== 0)
          throw new Error("dispatcher did not stop");
        finished = true;
        console.log("WASM control dispatcher lifecycle passed");
        process.exit(0);
      } catch (error) {
        fail(error);
      }
    }, 25);
  } catch (error) {
    fail(error);
  }
};

setTimeout(() => fail(new Error("WASM runtime initialization timed out")),
  10000);
