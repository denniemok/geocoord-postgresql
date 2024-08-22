select 'Sydney,33.86°S,151.21°E'::GeoCoord = 'Sydney,33.86°S,151.21°E'::GeoCoord;
select 'Sydney,33.86°S,151.21°E'::GeoCoord = 'syDneY,151.2100°E 33.8600°S'::GeoCoord;
select 'syDneY,151.2100°E 33.8600°S'::GeoCoord = 'Sydney,33.86°S,151.21°E'::GeoCoord;
select 'syDneY,151.2100°E 33.8600°S'::GeoCoord = 'Sydney,35.96°S,121.78°E'::GeoCoord;
select 'syDneY,151.2100°E 33.8600°S'::GeoCoord = 'Melbourne,33.86°S,151.21°E'::GeoCoord;