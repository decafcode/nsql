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
    expect(() => db.exec("insert into x values (null)")).toThrow(
      expect.objectContaining({ code: "SQLITE_CONSTRAINT_NOTNULL" })
    );
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

    expect(() => stmt.run([false as any])).toThrow(
      expect.objectContaining({ code: "ERR_INVALID_ARG_TYPE" })
    );
  });

  test("bind invalid object", function() {
    const db = new Database(":memory:");
    const stmt = db.prepare("select ?");

    expect(() => stmt.run([new Date(0) as any])).toThrow(
      expect.objectContaining({ code: "ERR_INVALID_ARG_TYPE" })
    );
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

describe("one", function() {
  test("returns undefined for empty result set", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select 1 where 0").one();

    expect(result).toBeUndefined();
  });

  test("param empty array", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select 1.0 as one").one([]);

    expect(result).toEqual({ one: 1.0 });
  });

  test("returns null", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select null as nil").one();

    expect(result).toEqual({ nil: null });
  });

  test("returns integral number", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select 1234. as real").one();

    expect(result).toEqual({ real: 1234 });
  });

  test("returns fractional number", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select 3.14 as real").one();

    // Do I need to do epsilon comparisons here or something? eh whatever

    expect(result).toEqual({ real: 3.14 });
  });

  test("returns bigint", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select 1234 as bigint").one();

    // Jest does not natively support BigInts yet

    expect(result).toHaveProperty("bigint");
    expect(result!.bigint).toEqual(1234n);
  });

  test("returns string", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select 'Hello, World!' as str").one();

    expect(result).toEqual({ str: "Hello, World!" });
  });

  test("returns UTF-8 Cyrillic", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select 'Здравствуйте' as str").one();

    expect(result).toEqual({ str: "Здравствуйте" });
  });

  test("returns UTF-8 non-BMP emoji", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select '😂' as str").one();

    expect(result).toEqual({ str: "😂" });
  });

  test("returns BLOB as ArrayBuffer", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select x'01020304' as blob").one();

    // Not sure how best to do this in Jest...

    expect(result).toHaveProperty("blob");

    const { blob } = result!;

    expect(blob).toBeInstanceOf(ArrayBuffer);

    const ary = new Uint8Array(blob as ArrayBuffer, 0);

    expect([...ary]).toEqual([1, 2, 3, 4]);
  });

  test("returns multiple columns", function() {
    // Leave out the BigInt and ArrayBuffer cases here since they're annoying

    const db = new Database(":memory:");
    const result = db
      .prepare("select null as a, 1234. as b, 'hello' as c")
      .one();

    expect(result).toEqual({ a: null, b: 1234, c: "hello" });
  });
});

describe("integer fidelity", function() {
  test("round-trip large 64-bit integer", function() {
    const num = 0x1020304050607080n;

    const db = new Database(":memory:");
    const stmt = db.prepare("select ? as num");
    const result = stmt.one([num]);

    expect(result!.num).toEqual(num);
  });

  test("exceed signed int64 limit", function() {
    // High bit of this positive 64-bit int is set.
    // This will fit in an unsigned 64-bit int, but not into an unsigned 64-bit
    // int of the kind that SQLite deals in.

    const num = 0x8070605040302010n;

    const db = new Database(":memory:");
    const stmt = db.prepare("select ? as num");

    expect(() => stmt.one([num])).toThrow(
      expect.objectContaining({ code: "ERR_VALUE_OUT_OF_RANGE" })
    );
  });

  test("exceed 64-bit storage", function() {
    // This just doesn't fit into any kind of 64-bit int at all

    const num = 0x102030405060708090n;

    const db = new Database(":memory:");
    const stmt = db.prepare("select ? as num");

    expect(() => stmt.one([num])).toThrow(
      expect.objectContaining({ code: "ERR_VALUE_OUT_OF_RANGE" })
    );
  });
});

describe("named binds", function() {
  test("named binds with one()", function() {
    // use @ sigils instead of the usual colon here so that it doesn't visually
    // clash with JavaScript key-value syntax.

    const db = new Database(":memory:");
    const stmt = db.prepare("select @c as c, @a as a, @b as b");
    const result = stmt.one({ "@a": null, "@b": 1234, "@c": "hello" });

    // This should effectively round-trip the input

    expect(result).toEqual({ a: null, b: 1234, c: "hello" });
  });
});

describe("all", function() {
  test("return no rows", function() {
    const db = new Database(":memory:");
    const result = db.prepare("select 1 where 0").all();

    expect(result).toHaveLength(0);
  });

  test("return multiple rows", function() {
    const data = [
      [3, "three"],
      [2, "two"],
      [1, "one"]
    ];

    const db = new Database(":memory:");

    db.exec("create table test (num real, str text)");

    const stmt = db.prepare("insert into test (num, str) values (?, ?)");

    data.forEach(datum => stmt.run(datum));

    const result = db.prepare("select num, str from test").all();

    expect(result).toHaveLength(3);
    expect(result).toContainEqual({ num: 1, str: "one" });
    expect(result).toContainEqual({ num: 2, str: "two" });
    expect(result).toContainEqual({ num: 3, str: "three" });
  });
});

describe("sql getter", function() {
  test("return sql", function() {
    const db = new Database(":memory:");
    const stmt = db.prepare("select   123, @abc");

    expect(stmt.sql).toBe("select   123, @abc");
  });

  test("closed statement", function() {
    const db = new Database(":memory:");
    const stmt = db.prepare("select 1");

    stmt.close();
    expect(typeof stmt.sql).toBe("string");
  });
});
