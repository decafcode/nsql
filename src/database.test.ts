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
