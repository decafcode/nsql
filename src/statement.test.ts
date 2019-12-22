import Database from ".";

describe("close", function() {
  test("close statement", function() {
    const db = new Database(":memory:");
    const stmt = db.prepare("select 1");

    stmt.close();
    stmt.close();
  });
});

describe("run", function() {
  test("run after close does not crash the process", function() {
    const db = new Database(":memory:");
    const stmt = db.prepare("select 1");

    stmt.close();
    expect(() => stmt.run()).toThrow();
  });

  test("bind nothing", function() {
    const db = new Database(":memory:");

    db.prepare("select 1").run();
  });

  // We rely on this construct to generate an exception if an expectation inside
  // our SQL is not met (e.g. check for the correct value type being bound).
  //
  // Something simpler like a divide by zero would be nice, but unfortunately
  // dividing by zero just yields SQL NULL.

  test("constraint violation throws", function() {
    const db = new Database(":memory:");

    db.exec("create table x (y integer not null)");
    expect(() => db.exec("insert into x values (null)")).toThrow();
  });

  test("bind null", function() {
    const db = new Database(":memory:");

    db.exec("create table x (y integer not null)");
    db.prepare(
      "insert into x select case typeof(?) when 'null' then 1 else null end"
    ).run([null]);
  });

  test("bind integer", function() {
    const db = new Database(":memory:");

    db.exec("create table x (y integer not null)");
    db.prepare(
      "insert into x select case typeof(?) when 'integer' then 1 else null end"
    ).run([1234n]);
  });

  test("bind real", function() {
    const db = new Database(":memory:");

    db.exec("create table x (y integer not null)");
    db.prepare(
      "insert into x select case typeof(?) when 'real' then 1 else null end"
    ).run([1234]);
  });

  test("bind text", function() {
    const db = new Database(":memory:");

    db.exec("create table x (y integer not null)");
    db.prepare(
      "insert into x select case typeof(?) when 'text' then 1 else null end"
    ).run(["1234"]);
  });

  test("bind blob", function() {
    const buf = Uint8Array.from([1, 2, 3, 4]).buffer;
    const db = new Database(":memory:");

    db.exec("create table x (y integer not null)");
    db.prepare(
      "insert into x select case typeof(?) when 'blob' then 1 else null end"
    ).run([buf]);
  });

  test("bind invalid primitive", function() {
    const db = new Database(":memory:");
    const stmt = db.prepare("select ?");

    expect(() => stmt.run([false as any])).toThrow();
  });

  test("bind invalid object", function() {
    const db = new Database(":memory:");
    const stmt = db.prepare("select ?");

    expect(() => stmt.run([new Date(0) as any])).toThrow();
  });
});

describe("run result", function() {
  test("check result.changes", function() {
    const db = new Database(":memory:");

    db.exec("create table x (y integer not null)");

    const insert = db.prepare("insert into x values (?)");

    for (const num of [1, 2, 3, 4, 5]) {
      insert.run([num]);
    }

    const result = db.prepare("update x set y = -1 where y % 2 = 0").run();

    expect(result.changes).toEqual(2);
  });

  test("check result.lastInsertRowId", function() {
    const db = new Database(":memory:");

    // Type INTEGER PRIMARY KEY makes `y` act as the ROWID for this table:
    // https://sqlite.org/lang_createtable.html#rowid
    db.exec("create table x (y integer primary key not null)");

    const result = db.prepare("insert into x values (?)").run([1234n]);

    expect(result.lastInsertRowid).toEqual(1234n);
  });
});
