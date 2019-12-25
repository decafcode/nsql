const Database = require("node-gyp-build")(__dirname + "/..");
const util = require("util");

Database.prototype[util.inspect.custom] = function(depth, options) {
  const { dbFilename } = this;

  return options.stylize(
    dbFilename !== "" ? `[Database ${dbFilename}]` : "[Temporary Database]",
    "special"
  );
};

Database._Statement.prototype[util.inspect.custom] = function(depth, options) {
  return options.stylize(`<${this.sql}>`, "special");
};

module.exports = Database;
