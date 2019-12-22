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
   * connection will result in an error (except for `close()`, which can safely
   * be called multiple times). If any statements that have been prepared for
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
}

export default Database;
