/**
 * SQLite NAPI binding. The {@link Database} class is the default export for
 * this module.
 *
 * @module SQLite
 */

/**
 * Bind parameter or result set value for an SQL statement.
 *
 * SQLite types are mapped to JavaScript types as follows:
 *
 * | SQLite    | JavaScript    |
 * |-----------|---------------|
 * | `NULL`    | `null`        |
 * | `INTEGER` | `bigint`      |
 * | `REAL`    | `number`      |
 * | `TEXT`    | `string`      |
 * | `BLOB`    | `ArrayBuffer` |
 *
 * - SQLite `INTEGER`s are 64-bit signed integers. Attempting to bind a BigInt
 *    whose magnitude is too great to store in a 64-bit signed integer
 *    representation will result in an error.
 * - JavaScript `number`s are always bound to statement parameters as double-
 *    precision floating point values, irrespective of whether or not they have
 *    a fractional component.
 */
export type SqlValue = null | number | bigint | string | ArrayBuffer;

/**
 * A collection of bind parameters suitable for passing to a prepared statement.
 * This can either be an array of `SqlValue`s whose values all conform to the
 * `SqlValue` definition.
 *
 * If an array is supplied then its elements will be bound to the statement's
 * positional parameters (or to named parameters in the order in which they
 * occur). If an object is supplied then its keys will be bound to the query's
 * named parameters, and an error will be raised if there are any keys that do
 * not correspond to a named parameter in the query.
 *
 * Please note that the symbol at the start of a bind parameter is considered to
 * be part of the parameter's name.
 */
export type BindParams = SqlValue[] | { [key: string]: SqlValue };

/**
 * A single row from an SQLite result set. See {@link SqlValue} for the data
 * type mapping between SQLite values and JavaScript values.
 */
export interface ResultRow {
  [key: string]: SqlValue;
}

/** Status information returned from a {@link Statement.run} call. */
export interface RunResult {
  /** Number of rows affected */
  changes: number;

  /** The ROWID value of the last inserted row, if applicable. */
  lastInsertRowid: bigint;
}

/**
 * An SQLite prepared statement.
 *
 * The methods of this class are used to bind some (possibly zero) parameters
 * to the prepared statement and then execute the statement.
 *
 * This class cannot be instantiated directly.
 *
 * @see SqlValue
 */
export declare class Statement {
  /**
   * Close prepared statement. Calling any methods on a closed statement will
   * result in an error, with the exception of further calls to `close()` which
   * will have no effect.
   */
  close(): undefined;

  /**
   * Execute a statement, returning status information.
   *
   * @param params Bind parameters (see {@link BindParams}).
   */
  run(params?: BindParams): RunResult;

  /**
   * Execute a statement, returning a single row or `undefined`.
   *
   * @param params Bind parameters (see {@link BindParams}).
   */
  one(params?: BindParams): ResultRow | undefined;

  /**
   * Execute a statement, returning an array of multiple (possibly zero) rows.
   *
   * @param params Bind parameters (see {@link BindParams}).
   */
  all(params?: BindParams): ResultRow[];

  /**
   * The original SQL used to prepare this statement, including placeholders.
   *
   * If the statement has been closed then a dummy string whose value should
   * not be relied upon is returned.
   *
   * This property is primarily provided for diagnostic purposes.
   */
  readonly sql: string;
}

/**
 * An SQLite database connection.
 */
declare class Database {
  /**
   * Open an SQLite database file.
   *
   * The only mode supported at the moment is opening a database file for read-
   * write access, creating it if it does not already exist. SQLite itself
   * understands the following paths:
   *
   * - If the special string `:memory:` is supplied then a transient in-memory
   *   database is opened. The contents of this database are lost when the
   *   database connection is closed.
   * - If the empty string is supplied then a transient database stored in
   *   a temporary file on disk is opened. This temporary file is deleted if
   *   the database connection is closed cleanly.
   * - Any other string is interpreted as a path on the local filesystem.
   *   SQLite will open a persistent database file at this location.
   *
   * @param uri Path to a database file, or `:memory:`, or the empty string.
   */
  constructor(uri: string);

  /**
   * Close the database connection. Calling any methods on a closed database
   * connection will result in an error, with the exception of further calls to
   * `close()` which will have no effect.
   *
   * If any statements that have been prepared for
   * this connection are still open then the operating system resources
   * associated with this connection are not released until they are all closed
   * or garbage collected.
   */
  close(): undefined;

  /**
   * Execute one SQL statement or multiple SQL statements separated by
   * semicolons. Results are discarded, and parameter binding is not possible;
   * use a prepared statement if you need to retrieve results or bind
   * parameters.
   *
   * This function is useful for running a sequence of SQL DDL commands to
   * initialize or upgrade an application's database schema.
   *
   * @param sql One or more SQL statements.
   */
  exec(sql: string): undefined;

  /**
   * Prepare an SQL statement.
   *
   * A prepared statement may end with a semicolon, although this is not
   * recommended. If a semicolon is present then it must be the last character
   * in the input string, otherwise an error will occur.
   *
   * Returns a prepared statement object which can be repeatedly invoked with
   * various bind parameters.
   *
   * SQLite supports several different kind of bind parameter syntax. The
   * details can be found on the following page:
   *
   * https://sqlite.org/lang_expr.html#varparam
   *
   * @param sql SQL statement, possibly including placeholders.
   */
  prepare(sql: string): Statement;

  /**
   * The absolute path to the file backing this database connection.
   *
   * For in-memory or temporary databases the value of this property is the
   * empty string. If the database has been closed then a dummy string whose
   * value should not be relied upon is returned.
   *
   * This property is primarily provided for diagnostic purposes.
   */
  readonly dbFilename: string;
}

export default Database;
