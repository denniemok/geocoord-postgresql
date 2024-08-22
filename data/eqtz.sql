select 'Sydney,33.86°S,151.21°E'::GeoCoord ~ 'Sydney,33.86°S,151.21°E'::GeoCoord;
select 'Sydney,33.86°S,151.21°E'::GeoCoord ~ 'Chuuk Islands,5.45°N,153.51°E'::GeoCoord;
select 'Chuuk Islands,5.45°N,153.51°E'::GeoCoord ~ 'Sydney,33.86°S,151.21°E'::GeoCoord;
select 'Chuuk Islands,5.45°N,153.51°E'::GeoCoord ~ 'Melbourne,37.84°S,144.95°E'::GeoCoord;
select 'Melbourne,37.84°S,144.95°E'::GeoCoord ~ 'Sydney,33.86°S,151.21°E'::GeoCoord;
select 'Melbourne,37.84°S,144.95°E'::GeoCoord ~ 'Alaska,70.2°S,144.0°W'::GeoCoord;