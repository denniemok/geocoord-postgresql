# GeoCoord Data Type for PostgreSQL

## Overview

This project introduces a new `GeoCoord` data type for PostgreSQL, implemented in C, to represent geographical coordinates. It includes input / output functions, comparison operators, and indexing support.

The implementation consists of two parts:
- `gcoord.c`: Contains the C functions for the `GeoCoord` data type.
- `gcoord.source`: Contains the SQL commands to install the `GeoCoord` data type in PostgreSQL.

## Compilation

PostgreSQL's extensibility model allows adding new data types to the server. To compile and install this project:

1. Install PostgreSQL from source (see [PostgreSQL installation guide](https://www.postgresql.org/docs/current/installation.html)).
2. Start the PostgreSQL server.
3. Copy `gcoord.c` and `gcoord.source` to `SOURCE_FOLDER/src/tutorial`.
4. Edit the `Makefile` in the directory and add `gcoord` to `MODULES` and `gcoord.sql` to `DATA_built`:

    ```makefile
    MODULES = complex funcs gcoord
    DATA_built = advanced.sql basics.sql complex.sql funcs.sql syscat.sql gcoord.sql
    ```

5. Run `make` to compile and install the data type.

To uninstall, use `DROP TYPE GeoCoord CASCADE`.

## Data Format

- A geographical coordinate has three parts: `LocationName`, `Latitude`, and `Longitude`.
- `LocationName` consists of one or more words separated by spaces, with each word being a sequence of letters.
- `Latitude` and `Longitude` consist of a coordinate value and a direction:
  - The coordinate value is a non-negative real number with up to four decimal places.
  - The latitude value must be ≤ 90, and the longitude value must be ≤ 180.
  - Latitude directions: 'N' (North) or 'S' (South).
  - Longitude directions: 'W' (West) or 'E' (East).

**Valid Examples:**
```
Melbourne,37.84°S,144.95°E
San Francisco,37.77°N,122.42°W
```

**Invalid Examples:**
```
Melbourne,37.84S,144.95E
Melbourne,37.84,144.95
Melbourne:37.84°S,144.95°E
37.84°S,144.95°E
```

## Operations

### GeoCoord1 = GeoCoord2

- Two geographical coordinates are equivalent if their `LocationName`, `Longitude`, and `Latitude` match in their canonical forms.
- The extension also supports `<>` operation.

### GeoCoord1 > GeoCoord2

- `GeoCoord1` is greater if its latitude is closer to the equator. If latitudes are equal, the one in the North is greater. 
- If latitudes are equal, `GeoCoord1` is greater if its longitude is closer to the prime meridian. If longitudes are equal, the one in the East is greater.
- If both latitude and longitude are equal, `LocationName` determines the ordering.
- The extension also supports `<`, `>`, `>=`, `<=` operations.

### GeoCoord1 ~ GeoCoord2

- Two coordinates are in the same time zone if their longitudes, divided by 15 and floored, are equal.
- Longitude direction is considered in the calculation.
- The extension also supports `!~` operation.

### Convert2DMS(GeoCoord)

Converts decimal coordinates to DMS (Degrees, Minutes, Seconds) format:

```
D = floor(A)
M = floor(60 × |A - D|)
S = floor(3600 × |A -D| - 60 × M)
```

- `A` is the coordinate value in latitude and longitude.
- If `M` or `S` is 0, it is not displayed.

### Ordering

- Ordering is primarily based on latitude and longitude.
- If both are equal, `LocationName` is used, sorted lexically and case-insensitively.

### Indexing

To create an indexed table using `GeoCoord`:

```sql
CREATE TABLE StoreInfo (
id INTEGER PRIMARY KEY,
location GeoCoord
);

INSERT INTO StoreInfo(id, location) VALUES
(1, 'Sydney,33.86°S,151.21°E'),
(2, 'Melbourne,37.84°S,144.95°E');

CREATE INDEX ON StoreInfo USING hash (location);

EXPLAIN ANALYZE SELECT * FROM StoreInfo WHERE location='Melbourne,37.84°S,144.95°E';
```

This will use a hash-based index for faster queries.