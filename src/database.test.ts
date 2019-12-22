import Database from ".";

describe("constructor", function() {
  test("open transient in-memory database", function() {
    new Database(":memory:");
  });

  test("call constructor without new", function() {
    expect(() => (Database as any)(":memory:")).toThrow();
  });

  test("open parameter type check", function() {
    expect(() => new (Database as any)()).toThrow();
    expect(() => new Database(123 as any)).toThrow();
  });

  test("open invalid path", function() {
    expect(() => new Database("/does/not/exist")).toThrow();
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

    expect(() => (db as any).exec()).toThrow();
    expect(() => db.exec(123 as any)).toThrow();
  });

  test("execute with semicolons", function() {
    const db = new Database(":memory:");

    db.exec("create table foo (val integer); create table bar (val integer)");
    db.exec("insert into bar values (1234)");
  });
});
