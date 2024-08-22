select 'Sydney,33.86°S,151.21°E'::GeoCoord > 'Sydney,35.86°S 150.21°E'::GeoCoord;
select 'Sydney,33.86°S,151.21°E'::GeoCoord > 'sydney,35.86°S,162.78°E'::GeoCoord;
select 'Sydney,35.86°S 150.21°E'::GeoCoord > 'sydney,35.86°S,162.78°E'::GeoCoord;
select 'Sydney,33.86°S,151.21°E'::GeoCoord > 'Sydney,33.86°S,151.21°E'::GeoCoord;
select 'melbourne,33.86°S 151.21°E'::GeoCoord > 'sydney,35.86°S,162.78°E'::GeoCoord;
select 'Sydney,33.86°S,151.21°E'::GeoCoord > 'melbourne,33.86°S 151.21°E'::GeoCoord;