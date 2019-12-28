#!/usr/bin/env node

const download = require("download");
const pkg = require("./package.json");

const targets = ["Visual Studio 2015", "Ubuntu", "macOS"];

async function main() {
  for (const target of targets) {
    console.log("Downloading build:", target);

    await download(
      `https://ci.appveyor.com/api/projects/decafcode/nsql/artifacts/${target}.zip?job=Image:%20${target}&tag=v${pkg.version}`,
      "prebuilds",
      { extract: true }
    );
  }
}

main().catch(e => {
  console.log(e);
  process.exit(1);
});
