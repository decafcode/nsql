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
}

export default Database;
