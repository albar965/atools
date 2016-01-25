drop table if exists scenery_area;

create table scenery_area
(
  scenery_area_id integer primary key,
  number integer not null,
  layer integer not null,
  title text,
  remote_path text,
  local_path text,
  active integer not null,
  required integer not null,
  exclude text
);

drop table if exists bgl_file;

create table bgl_file
(
  bgl_file_id integer primary key,
  scenery_area_id integer not null,
  bgl_create_time integer not null,
  file_modification_time integer not null,
  filename text not null,
  size integer not null,
foreign key(scenery_area_id) references scenery_area(scenery_area_id)
);
