-- *****************************************************************************
-- Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

-- ==================================================================================
-- Fill COM table

delete from com;

insert into com (airport_id, type, frequency, name)
select airport_id,
  case
    when c.communication_type = 'ACC' then 'CTR' -- Area Control Center
    when c.communication_type = 'ACP' then communication_type -- Airlift Command Post
    when c.communication_type = 'AIR' then communication_type -- Air to Air
    when c.communication_type = 'APP' then 'A' -- Approach Control
    when c.communication_type = 'ARR' then communication_type -- Arrival Control
    when c.communication_type = 'ASO' then 'ASOS' -- Automatic Surface Observing System (ASOS)
    when c.communication_type = 'ATI' then 'ATIS' -- Automatic Terminal Information Services (ATIS)
    when c.communication_type = 'AWI' then communication_type -- Airport Weather Information Broadcast (AWIB)
    when c.communication_type = 'AWO' then 'AWOS' -- Automatic Weather Observing Service (AWOS)
    when c.communication_type = 'AWS' then communication_type -- Aerodrome Weather Information Service (AWIS)
    when c.communication_type = 'CLD' then 'C' -- Clearance Delivery
    when c.communication_type = 'CPT' then 'CPT' -- Clearance, Pre-Taxi
    when c.communication_type = 'CTA' then communication_type -- Control Area (Terminal)
    when c.communication_type = 'CTL' then communication_type -- Control
    when c.communication_type = 'DEP' then 'D' -- Departure Control
    when c.communication_type = 'DIR' then communication_type -- Director (Approach Control Radar)
    when c.communication_type = 'EFS' then communication_type -- Enroute Flight Advisory Service (EFAS)
    when c.communication_type = 'EMR' then communication_type -- Emergency
    when c.communication_type = 'FSS' then 'FSS' -- Flight Service Station
    when c.communication_type = 'GCO' then communication_type -- Ground Comm Outlet
    when c.communication_type = 'GET' then communication_type -- Gate Control
    when c.communication_type = 'GND' then 'G' -- Ground Control
    when c.communication_type = 'HEL' then communication_type -- Helicopter Frequency
    when c.communication_type = 'INF' then communication_type -- Information
    when c.communication_type = 'MIL' then communication_type -- Military Frequency
    when c.communication_type = 'MUL' then 'MC' -- Multicom
    when c.communication_type = 'OPS' then communication_type -- Operations
    when c.communication_type = 'PAL' then communication_type -- Pilot Activated Lighting
    when c.communication_type = 'RDO' then communication_type -- Radio
    when c.communication_type = 'RDR' then communication_type -- Radar
    when c.communication_type = 'RFS' then communication_type -- Remote Flight Service Station (RFSS)
    when c.communication_type = 'RMP' then communication_type -- Ramp/Taxi Control
    when c.communication_type = 'RSA' then communication_type -- Airport Radar Service Area (ARSA)
    when c.communication_type = 'TCA' then communication_type -- Terminal Control Area (TCA)
    when c.communication_type = 'TMA' then communication_type -- Terminal Control Area (TMA)
    when c.communication_type = 'TML' then communication_type -- Terminal
    when c.communication_type = 'TRS' then communication_type -- Terminal Radar Service Area (TRSA)
    when c.communication_type = 'TWE' then communication_type -- Transcriber Weather Broadcast (TWEB)
    when c.communication_type = 'TWR' then 'T' -- Tower, Air Traffic Control
    when c.communication_type = 'UAC' then communication_type -- Upper Area Control
    when c.communication_type = 'UNI' then 'UC' -- Unicom
    when c.communication_type = 'VOL' then communication_type -- Volmet
    else null
  end as type,
  case
    -- Very High Frequency (30.000 kHz – 200 MHz) Ultra High Frequency (200 MHz – 3.000 MHz)
    -- Communication Channel for 8.33 kHz spacing
   when frequency_units in ('V', 'U', 'C') then round(communication_frequency * 1000)
   -- High Frequency (3.000 kHz – 30.000 kHz)
   when frequency_units = 'H' then round(communication_frequency)
   else 0
  end as frequency,
  --service_indicator,
  callsign as name
from tbl_airport_communication c
join airport a on c.airport_identifier = a.ident;


-- ==================================================================================
-- Update airport fields with COM information
-- Use lowest frequency if more than one available

-- Number of frequencies
update airport set num_com =
coalesce((select count(1)
from com c
where c.airport_id = airport.airport_id
group by c.airport_id), 0);

-- Tower
update airport set tower_frequency =
(select min(frequency)
from com c
where c.airport_id = airport.airport_id and c.type = 'T'
group by c.airport_id);

-- ATIS
update airport set atis_frequency =
(select min(frequency)
from com c
where c.airport_id = airport.airport_id and c.type = 'ATIS'
group by c.airport_id);

-- AWOS
update airport set awos_frequency =
(select min(frequency)
from com c
where c.airport_id = airport.airport_id and c.type = 'AWOS'
group by c.airport_id);

-- ASOS
update airport set asos_frequency =
(select min(frequency)
from com c
where c.airport_id = airport.airport_id and c.type = 'ASOS'
group by c.airport_id);

-- Unicom
update airport set unicom_frequency =
(select min(frequency)
from com c
where c.airport_id = airport.airport_id and c.type = 'UC'
group by c.airport_id);
