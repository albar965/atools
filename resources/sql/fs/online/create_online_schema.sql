-- *****************************************************************************
-- Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
-- ****************************************************************************/

drop table if exists client;

-- IVAO
-- https://doc.ivao.aero/apidocumentation:whazzup:fileformat
-- Name 	Client Types 	Description 	Unit
-- Callsign 	All 	The callsign of the connection. 	n/a
-- VID 	All 	The VID of the client involved in the connection. 	n/a
-- Name 	All 	The name of the person. 	n/a
-- Client Type 	All 	The type of the client connection. 	Enumeration
-- Frequency 	ATC 	The frequency the client is currently using. 	n/a
-- Frequency Cont… 	ATC 	Multiple frequencies are joined with an ampersand &. 	n/a
-- Latitude 	All 	The latitude of the current position of the connection. 	Decimal Degrees
-- Longitude 	All 	The longitude of the current position of the connection. 	Decimal Degrees
-- Altitude 	All 	The altitude of the current position of the connection. 	Feet
-- Groundspeed 	Pilot 	The groundspeed of the pilot. 	Knots
-- Flightplan: Aircraft 	Pilot 	According to ICAO flightplan specifications. Example: 1/C172/L-CS/C 	n/a
-- Flightplan: Cruising Speed 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: Departure Aerodrome 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: Cruising Level 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: Destination Aerodrome 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Server 	All 	The name of the server to which this client is connected. 	n/a
-- Protocol 	All 	The protocol revision the client is using. 	n/a
-- Combined Rating 	All 	Deprecated: use the separated rating fields instead. 	n/a
-- Transponder Code 	Pilot 	The transponder code set by the pilot. 	n/a
-- Facility Type 	ATC 	The facility provided by the ATC. 	Enumeration
-- Visual Range 	ATC 	The range of the ATC radar. 	NM
-- Flightplan: revision 	Pilot 	Sequence number of the flightplan. 	n/a
-- Flightplan: flight rules 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: departure time 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: actual departure time 	Pilot 	The actual departure time. Reserved for future use 	n/a
-- Flightplan: EET (hours) 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: EET (minutes) 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: endurance (hours) 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: endurance (minutes) 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: Alternate Aerodrome 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: item 18 (other info) 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: route 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- unused 	- 	- 	-
-- unused 	- 	- 	-
-- ATIS 	ATC 	The ATIS set by the controller. Deprecated: will be replaced by Airport based ATIS 	n/a
-- ATIS Time 	ATC 	The time the ATIS was defined. Deprecated: will be replaced by Airport based ATIS 	n/a
-- Connection Time 	All 	The time the client connected to the network. 	n/a
-- Software Name 	All 	The name of the software product the client is using. 	n/a
-- Software Version 	All 	The version of the software product the client is using. 	n/a
-- Administrative Version 	All 	The administrative rating of the client. 	Enumeration
-- ATC/Pilot Version 	All 	The ATC or Pilot rating of the client. 	Enumeration
-- Flightplan: 2nd Alternate Aerodrome 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: Type of Flight 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Flightplan: Persons on Board 	Pilot 	According to ICAO flightplan specifications. 	n/a
-- Heading 	Pilot 	The heading of the plane. 	Degrees
-- On ground 	Pilot 	A flag indicating if the plane is on ground or not. 	Boolean
-- Simulator 	Pilot 	The simulator used by the pilot. 	Enumeration
-- Plane 	Pilot 	The MTL identification of the plane with which the pilot is flying. 	n/a
--
-- VATSIM
-- callsign:cid:realname:clienttype:frequency:latitude:longitude:altitude:groundspeed:planned_aircraft:
-- planned_tascruise:planned_depairport:planned_altitude:planned_destairport:server:protrevision:rating:transponder:
-- facilitytype:visualrange:planned_revision:planned_flighttype:planned_deptime:planned_actdeptime:planned_hrsenroute:
-- planned_minenroute:planned_hrsfuel:planned_minfuel:planned_altairport:planned_remarks:planned_route:
-- planned_depairport_lat:planned_depairport_lon:planned_destairport_lat:planned_destairport_lon:atis_message:
-- time_last_atis_received:time_logon:heading:QNH_iHg:QNH_Mb:
create table client
(
  client_id integer primary key,
  callsign varchar(200),
  vid varchar(200),
  name varchar(200)  collate nocase,
  prefile integer,
  client_type  varchar(200),
  groundspeed varchar(200),
  flightplan_aircraft varchar(200),
  flightplan_cruising_speed varchar(200),
  flightplan_departure_aerodrome varchar(200),
  flightplan_cruising_level varchar(200),
  flightplan_destination_aerodrome varchar(200),
  server varchar(200),
  protocol varchar(200),
  combined_rating varchar(200),
  transponder_code varchar(200),
  facility_type integer,
  visual_range integer,                                       --Nm
  flightplan_revision varchar(200),
  flightplan_flight_rules varchar(200),
  flightplan_departure_time varchar(200),
  flightplan_actual_departure_time varchar(200),
  flightplan_enroute_minutes integer,
  flightplan_endurance_minutes integer,
  flightplan_alternate_aerodrome varchar(200),
  flightplan_other_info varchar(200),
  flightplan_route varchar(200),
  connection_time varchar(200),
  software_name varchar(200),
  software_version varchar(200),
  administrative_rating integer,
  atc_pilot_rating integer,
  flightplan_2nd_alternate_aerodrome varchar(200),
  flightplan_type_of_flight varchar(200),
  flightplan_persons_on_board integer,
  heading integer, -- degree
  on_ground integer,
  simulator integer,
  plane varchar(200),
  qnh_mb integer,
  altitude integer,                          -- ft
  lonx double not null,
  laty double not null
);

drop table if exists atc;

create table atc
(
  atc_id integer primary key,
  callsign varchar(200),
  vid varchar(200),
  name varchar(200) collate nocase,
  type varchar(15),               -- Same as boundary.type
  com_type varchar(30),           -- Same as boundary.com_type
  client_type  varchar(200),
  frequency varchar(200),
  server varchar(200),
  protocol varchar(200),
  combined_rating varchar(200),
  facility_type integer,
  visual_range integer,           --Nm
  atis varchar(200),
  atis_time varchar(200),
  connection_time varchar(200),
  software_name varchar(200),
  software_version varchar(200),
  administrative_rating integer,
  atc_pilot_rating integer,
  simulator integer,
  qnh_mb integer,
  lonx double not null,
  laty double not null,
  radius integer not null,        --Nm - painting radius for circle
  max_lonx double not null,       -- Bounding rectangle
  max_laty double not null,       -- "
  min_lonx double not null,       -- Bounding rectangle
  min_laty double not null,       -- "
  geometry blob                   -- Pre calculated geometry (circle)
);

drop table if exists server;

-- IVAO
-- Ident 		The identification of the server (unique). 		n/a
-- Host name / IP 		The host name or IP address of the server 		n/a
-- Location 		The physical location of the server. 		n/a
-- Name 		The descriptive name of the server. 		n/a
-- Client Connections Allowed. 		A flag indicating if connections to this server are allowed or not. 		Boolean
-- Maximum Connection 		The maximum number of connections to this server. 		n/a
--
-- VATSIM
-- ident:hostname_or_IP:location:name:clients_connection_allowed:
--
-- VATSIM Voice server
-- hostname_or_IP:location:name:clients_connection_allowed:type_of_voice_server:
create table server
(
  server_id integer primary key,
  ident varchar(200),
  hostname varchar(200),
  location varchar(200),
  name varchar(200),
  client_connections_allowed integer,
  allowed_connections integer,
  voice_type varchar(200)
);

drop table if exists airport;

-- IVAO
-- Name 	Description 	Unit
-- ICAO 	The ICAO code of the airport. 	n/a
-- ATIS 	The ATIS of the airport. 	n/a
create table airport
(
  airport_id integer primary key,
  ident varchar(200),
  atis varchar(200)
);
