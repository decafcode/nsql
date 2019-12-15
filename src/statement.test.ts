import Database from ".";

describe("close", function() {
  test("close statement", function() {
    const db = new Database(":memory:");
    const stmt = db.prepare("select 1");

    stmt.close();
    stmt.close();
  });
});
