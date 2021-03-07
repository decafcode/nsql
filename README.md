# @decafcode/sqlite

<img src="https://ci.appveyor.com/api/projects/status/github/decafcode/nsql?branch=master&svg=true">

Synchronous N-API interface to SQLite.

The version of SQLite currently bundled with this extension is 3.34.1.

# Features

- Statically links its own copy of SQLite for ease of deployment.
- Provides pre-built binaries with its npm distribution packages for ease of
  deployment. The current release includes pre-built binaries for the following
  JavaScript platforms:
  - Node.js on 64-bit Linux
  - Node.js on 64-bit Windows
  - Node.js on 64-bit macOS
- SQLite INTEGERs are always returned as JavaScript BigInts and JavaScript
  numbers are always bound as SQLite REALs. This prevents silent corruption of
  large integral values.
- Exposes an [N-API](https://nodejs.org/dist/latest-v12.x/docs/api/n-api.html)
  interface to its host JavaScript runtime, which provides improved binary
  portability between runtime versions compared to the original Node.js C++
  extension API.

# Usage

To use @decafcode/sqlite from your project simply install it using your
preferred package manager:

```
$ npm install --save @decafcode/sqlite
```

or

```
$ yarn add @decafcode/sqlite
```

TypeScript definitions with TSDoc comments are included in the package. You may
also consult the [online copy](https://decafcode.github.io/nsql/) of the
project's Typedoc documentation output.

# Contributing

See [INTERNALS.md](INTERNALS.md) for details about the internals of this
project.

# Example

```typescript
import Database from "@decafcode/sqlite";

function loadEmployees(path: string, departmentId: bigint): Employee[] {
  const db = new Database(path);

  const stmt = db.prepare(
    "select id, name from employee where department_id = ?"
  );

  const rows = stmt.all([departmentId]);
  const objects = new Array();

  stmt.close();

  for (const row of rows) {
    objects.push(new Employee(row.id, row.name));
  }

  db.close();

  return objects;
}
```

## License

MIT
