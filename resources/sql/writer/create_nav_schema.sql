drop table if exists waypoint;

create table waypoint
(
  waypoint_id integer primary key,
  file_id integer not null,
  ident text,
  region text,
  airport_id integer,
  type text not null,
  mag_var real not null,
  lonx real not null, 
  laty real not null,
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

drop table if exists vor;

create table vor
(
  vor_id integer primary key,
  file_id integer not null,
  ident text not null,
  name text not null,
  region text,
  airport_id integer,
  type text not null,
  frequency integer not null, 
  range integer not null, 
  mag_var real  not null,
  dme_only integer not null,
  dme_altitude integer,
  dme_lonx real, 
  dme_laty real,
  altitude integer not null,
  lonx real not null, 
  laty real not null,
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

drop table if exists ndb;

create table ndb
(
  ndb_id integer primary key,
  file_id integer not null,
  ident text,
  name text not null,
  region text,
  airport_id integer,
  type text not null,
  frequency integer not null, 
  range integer not null, 
  mag_var real  not null,
  altitude integer not null,
  lonx real not null, 
  laty real not null,
foreign key(file_id) references bgl_file(bgl_file_id),
foreign key(airport_id) references airport(airport_id)
);

drop table if exists marker;

create table marker
(
  marker_id integer primary key,
  file_id integer not null,
  ident text,
  region text,
  type text not null,
  altitude integer not null,
  lonx real not null, 
  laty real not null,
foreign key(file_id) references bgl_file(bgl_file_id)
);

drop table if exists ils;

create table ils
(
  ils_id integer primary key,
  ident text not null,
  name text,
  region text,
  frequency integer not null, 
  range integer not null, 
  mag_var real  not null,
  is_backcourse integer not null,
  dme_range integer, 
  dme_altitude integer,
  dme_lonx real, 
  dme_laty real,
  gs_range integer, 
  gs_pitch real,
  gs_altitude integer,
  gs_lonx real, 
  gs_laty real,
  loc_runway_end_id integer,
  loc_heading real not null,
  loc_width real not null,
  altitude integer not null,
  lonx real not null, 
  laty real not null,
foreign key(loc_runway_end_id) references runway_end(runway_end_id)
);

drop table if exists temp_route;

create table temp_route
(
  temp_route_id integer primary key,
  waypoint_id integer not null,
  name text not null,
  type text not null,
  next_type text,
  next_ident text,
  next_region text,
  next_airport_ident,
  next_minimum_altitude integer,
  previous_type text,
  previous_ident text,
  previous_region text,
  previous_airport_ident,
  previous_minimum_altitude integer,
foreign key(waypoint_id) references waypoint(waypoint_id)
);

drop table if exists route;

create table route
(
  route_id integer primary key,
  route_name text not null,
  route_type text not null,
  route_fragment_no integer not null,
  sequence_no integer not null,
  from_waypoint_id integer not null,
  to_waypoint_id integer not null,
  minimum_altitude integer,
foreign key(from_waypoint_id) references waypoint(waypoint_id),
foreign key(to_waypoint_id) references waypoint(waypoint_id)
);
