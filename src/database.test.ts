import Database from ".";

describe("constructor", function() {
  test("open transient in-memory database", function() {
    new Database(":memory:");
  });

  test("call constructor without new", function() {
    expect(() => (Database as any)(":memory:")).toThrow();
  });

  test("open parameter type check", function() {
    expect(() => new (Database as any)()).toThrow(
      expect.objectContaining({ code: "ERR_INVALID_ARG_TYPE" })
    );

    expect(() => new Database(123 as any)).toThrow(
      expect.objectContaining({ code: "ERR_INVALID_ARG_TYPE" })
    );
  });

  test("open invalid path", function() {
    expect(() => new Database("/does/not/exist")).toThrow(
      expect.objectContaining({ code: "SQLITE_CANTOPEN" })
    );
  });
});

describe("close", function() {
  test("close handle", function() {
    const db = new Database(":memory:");

    db.close();
    db.close();
  });
});

describe("exec", function() {
  test("execute sql", function() {
    const db = new Database(":memory:");

    db.exec("select 1");
  });

  test("execute after close does not crash the process", function() {
    const db = new Database(":memory:");

    db.close();
    expect(() => db.exec("select 1")).toThrow();
  });

  test("execute parameter type check", function() {
    const db = new Database(":memory:");

    expect(() => (db as any).exec()).toThrow(
      expect.objectContaining({ code: "ERR_INVALID_ARG_TYPE" })
    );

    expect(() => db.exec(123 as any)).toThrow(
      expect.objectContaining({ code: "ERR_INVALID_ARG_TYPE" })
    );
  });

  test("execute with semicolons", function() {
    const db = new Database(":memory:");

    db.exec("create table foo (val integer); create table bar (val integer)");
    db.exec("insert into bar values (1234)");
  });
});

describe("prepare", function() {
  test("prepare sql", function() {
    const db = new Database(":memory:");

    db.prepare("select 1");
  });

  test("prepare after close does not crash the process", function() {
    const db = new Database(":memory:");

    db.close();
    expect(() => db.prepare("select 1")).toThrow();
  });

  test("prepare type check", function() {
    const db = new Database(":memory:");

    expect(() => db.prepare(123 as any)).toThrow(
      expect.objectContaining({ code: "ERR_INVALID_ARG_TYPE" })
    );
  });

  test("prepare invalid sql", function() {
    const db = new Database(":memory:");

    expect(() => db.prepare("invalid")).toThrow(
      expect.objectContaining({ code: "SQLITE_ERROR" })
    );
  });

  test("prepare error contains detailed diagnostics", function() {
    const db = new Database(":memory:");

    expect(() => db.prepare("invalid_xyz")).toThrowError(/invalid_xyz/);
  });

  test("prepare trailing chars", function() {
    const db = new Database(":memory:");

    expect(() => db.prepare("select 1; ")).toThrow(
      expect.objectContaining({ code: "ERR_INVALID_ARG_VALUE" })
    );
  });
});

describe("dbName", function() {
  test("in-memory database", function() {
    const db = new Database(":memory:");

    expect(db.dbFilename).toBe("");
  });

  test("temporary file database", function() {
    const db = new Database("");

    expect(db.dbFilename).toBe("");
    db.close();
  });

  test("closed database", function() {
    const db = new Database(":memory:");

    db.close();
    expect(typeof db.dbFilename).toBe("string");
  });

  test("read-only", function() {
    const db = new Database(":memory:");

    expect(() => ((db as any).dbFilename = "error")).toThrow();
  });
});
