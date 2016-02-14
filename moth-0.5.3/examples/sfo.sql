# MySQL dump 8.14
#
# Host: localhost    Database: weather
#--------------------------------------------------------
# Server version	3.23.41

#
# Table structure for table 'sfo'
#

CREATE TABLE sfo (
  cycle datetime NOT NULL default '0000-00-00 00:00:00',
  temperature float default NULL,
  dewpoint float default NULL,
  pressure float default NULL,
  PRIMARY KEY  (cycle)
);

#
# Dumping data for table 'sfo'
#

INSERT INTO sfo VALUES ('2002-06-01 01:00:00',19.4,10.6,29.79);
INSERT INTO sfo VALUES ('2002-06-01 02:00:00',17.8,10.6,29.79);
INSERT INTO sfo VALUES ('2002-06-01 03:00:00',14.4,10,29.82);
INSERT INTO sfo VALUES ('2002-06-01 04:00:00',13.9,10,29.81);
INSERT INTO sfo VALUES ('2002-06-01 05:00:00',13.9,10,29.82);
INSERT INTO sfo VALUES ('2002-06-01 06:00:00',13.3,10,29.83);
INSERT INTO sfo VALUES ('2002-06-01 07:00:00',13.9,10,29.82);
INSERT INTO sfo VALUES ('2002-06-01 08:00:00',13.3,10,29.81);
INSERT INTO sfo VALUES ('2002-06-01 09:00:00',13.3,9.4,29.81);
INSERT INTO sfo VALUES ('2002-06-01 10:00:00',13.9,9.4,29.81);
INSERT INTO sfo VALUES ('2002-06-01 11:00:00',13.3,10,29.82);
INSERT INTO sfo VALUES ('2002-06-01 12:00:00',13.3,8.9,29.82);
INSERT INTO sfo VALUES ('2002-06-01 13:00:00',13.3,9.4,29.83);
INSERT INTO sfo VALUES ('2002-06-01 14:00:00',13.3,9.4,29.84);
INSERT INTO sfo VALUES ('2002-06-01 15:00:00',13.9,9.4,29.85);
INSERT INTO sfo VALUES ('2002-06-01 16:00:00',13.9,9.4,29.89);
INSERT INTO sfo VALUES ('2002-06-01 17:00:00',13.3,9.4,29.9);
INSERT INTO sfo VALUES ('2002-06-01 18:00:00',13.9,9.4,29.92);
INSERT INTO sfo VALUES ('2002-06-01 19:00:00',13.9,9.4,29.93);
INSERT INTO sfo VALUES ('2002-06-01 20:00:00',13.9,9.4,29.95);
INSERT INTO sfo VALUES ('2002-06-01 21:00:00',13.3,9.4,29.95);
INSERT INTO sfo VALUES ('2002-06-01 22:00:00',14.4,9.4,29.93);
INSERT INTO sfo VALUES ('2002-06-01 23:00:00',14.4,9.4,29.93);
INSERT INTO sfo VALUES ('2002-06-02 00:00:00',14.4,9.4,29.94);
INSERT INTO sfo VALUES ('2002-06-02 01:00:00',13.9,9.4,29.93);
INSERT INTO sfo VALUES ('2002-06-02 02:00:00',13.3,9.4,29.93);
INSERT INTO sfo VALUES ('2002-06-02 03:00:00',12.8,9.4,29.93);
INSERT INTO sfo VALUES ('2002-06-02 04:00:00',12.2,8.9,29.91);
INSERT INTO sfo VALUES ('2002-06-02 05:00:00',12.2,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-02 06:00:00',12.2,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-02 07:00:00',12.2,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-02 08:00:00',11.7,8.9,29.93);
INSERT INTO sfo VALUES ('2002-06-02 09:00:00',11.7,8.9,29.93);
INSERT INTO sfo VALUES ('2002-06-02 10:00:00',11.7,8.3,29.92);
INSERT INTO sfo VALUES ('2002-06-02 11:00:00',11.7,8.3,29.91);
INSERT INTO sfo VALUES ('2002-06-02 12:00:00',11.1,8.3,29.92);
INSERT INTO sfo VALUES ('2002-06-02 13:00:00',11.1,8.3,29.94);
INSERT INTO sfo VALUES ('2002-06-02 14:00:00',11.1,8.3,29.94);
INSERT INTO sfo VALUES ('2002-06-02 16:00:00',12.2,8.3,29.96);
INSERT INTO sfo VALUES ('2002-06-02 17:00:00',12.8,8.9,29.97);
INSERT INTO sfo VALUES ('2002-06-02 18:00:00',13.3,8.9,29.97);
INSERT INTO sfo VALUES ('2002-06-02 19:00:00',14.4,8.9,29.96);
INSERT INTO sfo VALUES ('2002-06-02 20:00:00',16.7,8.9,29.96);
INSERT INTO sfo VALUES ('2002-06-02 21:00:00',15.6,8.9,29.96);
INSERT INTO sfo VALUES ('2002-06-02 22:00:00',15.6,8.9,29.96);
INSERT INTO sfo VALUES ('2002-06-02 23:00:00',15.6,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-03 00:00:00',14.4,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-03 01:00:00',13.3,8.9,29.93);
INSERT INTO sfo VALUES ('2002-06-03 02:00:00',13.3,8.9,29.92);
INSERT INTO sfo VALUES ('2002-06-03 03:00:00',12.2,8.9,29.92);
INSERT INTO sfo VALUES ('2002-06-03 04:00:00',12.2,8.9,29.93);
INSERT INTO sfo VALUES ('2002-06-03 05:00:00',12.2,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-03 06:00:00',12.2,8.9,29.95);
INSERT INTO sfo VALUES ('2002-06-03 07:00:00',12.2,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-03 08:00:00',11.7,8.9,29.93);
INSERT INTO sfo VALUES ('2002-06-03 09:00:00',11.7,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-03 10:00:00',11.1,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-03 11:00:00',11.1,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-03 12:00:00',11.1,8.9,29.94);
INSERT INTO sfo VALUES ('2002-06-03 13:00:00',11.1,8.9,29.95);
INSERT INTO sfo VALUES ('2002-06-03 14:00:00',11.1,9.4,29.96);
INSERT INTO sfo VALUES ('2002-06-03 15:00:00',11.1,8.9,29.97);
INSERT INTO sfo VALUES ('2002-06-04 00:00:00',17.2,9.4,29.95);
INSERT INTO sfo VALUES ('2002-06-04 01:00:00',17.2,9.4,29.94);
INSERT INTO sfo VALUES ('2002-06-04 02:00:00',16.7,8.3,29.94);
INSERT INTO sfo VALUES ('2002-06-04 03:00:00',15,8.3,29.94);
INSERT INTO sfo VALUES ('2002-06-04 04:00:00',15,7.8,29.96);
INSERT INTO sfo VALUES ('2002-06-04 05:00:00',13.9,7.8,29.97);
INSERT INTO sfo VALUES ('2002-06-04 06:00:00',13.9,7.8,29.97);
INSERT INTO sfo VALUES ('2002-06-04 07:00:00',13.3,7.8,29.97);
INSERT INTO sfo VALUES ('2002-06-04 09:00:00',13.3,8.3,29.96);
INSERT INTO sfo VALUES ('2002-06-04 10:00:00',13.3,8.9,29.96);
INSERT INTO sfo VALUES ('2002-06-04 11:00:00',12.2,8.3,29.96);
INSERT INTO sfo VALUES ('2002-06-04 12:00:00',12.8,8.3,29.96);
INSERT INTO sfo VALUES ('2002-06-04 13:00:00',12.8,8.9,29.98);
INSERT INTO sfo VALUES ('2002-06-04 14:00:00',13.9,8.9,29.99);
INSERT INTO sfo VALUES ('2002-06-04 15:00:00',16.1,9.4,29.99);
INSERT INTO sfo VALUES ('2002-06-04 16:00:00',17.2,10.6,30);
INSERT INTO sfo VALUES ('2002-06-04 17:00:00',18.3,12.2,30);
INSERT INTO sfo VALUES ('2002-06-04 18:00:00',20,12.8,30);
INSERT INTO sfo VALUES ('2002-06-04 19:00:00',20,10.6,30);
INSERT INTO sfo VALUES ('2002-06-04 20:00:00',21.1,11.1,29.98);
INSERT INTO sfo VALUES ('2002-06-04 21:00:00',20.6,11.1,29.98);
INSERT INTO sfo VALUES ('2002-06-04 22:00:00',20.6,11.7,29.97);
INSERT INTO sfo VALUES ('2002-06-04 23:00:00',20,11.1,29.96);
INSERT INTO sfo VALUES ('2002-06-05 03:00:00',16.1,10.6,29.95);
INSERT INTO sfo VALUES ('2002-06-05 01:00:00',19.4,10.6,29.94);
INSERT INTO sfo VALUES ('2002-06-05 00:00:00',18.9,11.1,29.96);
INSERT INTO sfo VALUES ('2002-06-05 02:00:00',18.3,10.6,29.94);
INSERT INTO sfo VALUES ('2002-06-05 04:00:00',15,10.6,29.95);
INSERT INTO sfo VALUES ('2002-06-05 05:00:00',15,10.6,29.96);
INSERT INTO sfo VALUES ('2002-06-05 06:00:00',15,10.6,29.97);
INSERT INTO sfo VALUES ('2002-06-05 07:00:00',13.9,10,29.97);
INSERT INTO sfo VALUES ('2002-06-05 08:00:00',14.4,10.6,29.96);
INSERT INTO sfo VALUES ('2002-06-05 09:00:00',14.4,10,29.96);
INSERT INTO sfo VALUES ('2002-06-05 10:00:00',13.9,10,29.95);
INSERT INTO sfo VALUES ('2002-06-05 11:00:00',13.3,10,29.95);
INSERT INTO sfo VALUES ('2002-06-05 12:00:00',13.9,10,29.95);
INSERT INTO sfo VALUES ('2002-06-05 13:00:00',13.9,10,29.96);
INSERT INTO sfo VALUES ('2002-06-05 14:00:00',16.1,10.6,29.97);
INSERT INTO sfo VALUES ('2002-06-05 15:00:00',18.9,11.7,29.98);
INSERT INTO sfo VALUES ('2002-06-05 16:00:00',18.3,13.9,29.99);
INSERT INTO sfo VALUES ('2002-06-05 17:00:00',19.4,13.9,29.98);
INSERT INTO sfo VALUES ('2002-06-05 18:00:00',21.1,15.6,29.98);
INSERT INTO sfo VALUES ('2002-06-05 19:00:00',23.9,14.4,29.96);
INSERT INTO sfo VALUES ('2002-06-05 20:00:00',30,12.8,29.95);
INSERT INTO sfo VALUES ('2002-06-05 21:00:00',26.1,12.2,29.94);
INSERT INTO sfo VALUES ('2002-06-05 22:00:00',23.9,12.2,29.95);
INSERT INTO sfo VALUES ('2002-06-05 23:00:00',23.9,12.2,29.94);
INSERT INTO sfo VALUES ('2002-06-06 00:00:00',23.3,12.2,29.92);
INSERT INTO sfo VALUES ('2002-06-06 01:00:00',20.6,12.2,29.9);
INSERT INTO sfo VALUES ('2002-06-06 02:00:00',18.9,11.7,29.89);
INSERT INTO sfo VALUES ('2002-06-06 03:00:00',17.2,11.7,29.9);
INSERT INTO sfo VALUES ('2002-06-06 04:00:00',15.6,11.1,29.91);
INSERT INTO sfo VALUES ('2002-06-06 05:00:00',15,10.6,29.91);
INSERT INTO sfo VALUES ('2002-06-06 06:00:00',13.9,10,29.92);
INSERT INTO sfo VALUES ('2002-06-06 08:00:00',13.3,9.4,29.92);
INSERT INTO sfo VALUES ('2002-06-06 07:00:00',13.3,10,29.92);
INSERT INTO sfo VALUES ('2002-06-06 09:00:00',13.3,9.4,29.91);
INSERT INTO sfo VALUES ('2002-06-06 10:00:00',12.8,9.4,29.9);
INSERT INTO sfo VALUES ('2002-06-06 11:00:00',12.8,9.4,29.89);
INSERT INTO sfo VALUES ('2002-06-06 12:00:00',12.8,8.9,29.88);
INSERT INTO sfo VALUES ('2002-06-06 13:00:00',12.2,9.4,29.89);
INSERT INTO sfo VALUES ('2002-06-06 14:00:00',13.3,9.4,29.91);
INSERT INTO sfo VALUES ('2002-06-06 15:00:00',15.6,9.4,29.91);
INSERT INTO sfo VALUES ('2002-06-06 16:00:00',18.9,10,29.92);
INSERT INTO sfo VALUES ('2002-06-06 15:56:00',18.9,10,29.92);
INSERT INTO sfo VALUES ('2002-06-06 17:00:00',20,10,29.91);
INSERT INTO sfo VALUES ('2002-06-06 18:00:00',18.3,9.4,29.91);
INSERT INTO sfo VALUES ('2002-06-06 19:00:00',18.3,9.4,29.9);
INSERT INTO sfo VALUES ('2002-06-06 20:00:00',18.9,8.9,29.9);
INSERT INTO sfo VALUES ('2002-06-06 21:00:00',18.9,8.9,29.89);
INSERT INTO sfo VALUES ('2002-06-06 22:00:00',18.9,8.9,29.89);
INSERT INTO sfo VALUES ('2002-06-06 23:00:00',18.3,8.9,29.88);
INSERT INTO sfo VALUES ('2002-06-07 00:00:00',17.8,8.3,29.86);
INSERT INTO sfo VALUES ('2002-06-07 01:00:00',17.2,8.3,29.85);
INSERT INTO sfo VALUES ('2002-06-07 02:00:00',16.1,8.3,29.84);
INSERT INTO sfo VALUES ('2002-06-07 03:00:00',14.4,8.3,29.84);
INSERT INTO sfo VALUES ('2002-06-07 04:00:00',13.9,8.3,29.84);
INSERT INTO sfo VALUES ('2002-06-07 05:00:00',13.9,8.3,29.85);
INSERT INTO sfo VALUES ('2002-06-07 06:00:00',13.3,8.3,29.86);
INSERT INTO sfo VALUES ('2002-06-07 07:00:00',12.2,7.8,29.85);
INSERT INTO sfo VALUES ('2002-06-07 08:00:00',12.2,7.8,29.85);
INSERT INTO sfo VALUES ('2002-06-07 09:00:00',11.7,7.8,29.86);
INSERT INTO sfo VALUES ('2002-06-07 10:00:00',11.1,7.8,29.85);
INSERT INTO sfo VALUES ('2002-06-07 11:00:00',11.1,8.3,29.85);
INSERT INTO sfo VALUES ('2002-06-07 12:00:00',11.1,7.8,29.84);
INSERT INTO sfo VALUES ('2002-06-07 13:00:00',11.1,8.3,29.85);
INSERT INTO sfo VALUES ('2002-06-07 14:00:00',11.7,8.3,29.86);
INSERT INTO sfo VALUES ('2002-06-07 15:00:00',12.8,8.3,29.87);
INSERT INTO sfo VALUES ('2002-06-07 16:00:00',14.4,8.3,29.88);
INSERT INTO sfo VALUES ('2002-06-07 17:00:00',16.1,8.3,29.87);
INSERT INTO sfo VALUES ('2002-06-07 18:00:00',17.2,8.9,29.86);
INSERT INTO sfo VALUES ('2002-06-07 19:00:00',17.2,8.9,29.87);
INSERT INTO sfo VALUES ('2002-06-07 20:00:00',17.2,8.3,29.87);
INSERT INTO sfo VALUES ('2002-06-07 21:00:00',17.2,8.3,29.86);
INSERT INTO sfo VALUES ('2002-06-07 22:00:00',16.7,7.8,29.85);
INSERT INTO sfo VALUES ('2002-06-07 23:00:00',16.1,7.8,29.84);
INSERT INTO sfo VALUES ('2002-06-08 00:00:00',16.7,7.8,29.83);
INSERT INTO sfo VALUES ('2002-06-08 01:00:00',15.6,7.8,29.82);
INSERT INTO sfo VALUES ('2002-06-08 02:00:00',15.6,7.2,29.8);
INSERT INTO sfo VALUES ('2002-06-08 03:00:00',13.9,7.2,29.81);
INSERT INTO sfo VALUES ('2002-06-08 04:00:00',13.3,7.2,29.81);
INSERT INTO sfo VALUES ('2002-06-08 05:00:00',13.9,7.2,29.83);
INSERT INTO sfo VALUES ('2002-06-08 06:00:00',13.3,7.2,29.83);
INSERT INTO sfo VALUES ('2002-06-08 07:00:00',12.2,7.2,29.83);
INSERT INTO sfo VALUES ('2002-06-08 08:00:00',12.2,7.2,29.83);
INSERT INTO sfo VALUES ('2002-06-08 09:00:00',11.7,7.2,29.83);
INSERT INTO sfo VALUES ('2002-06-08 10:00:00',11.7,7.2,29.83);
INSERT INTO sfo VALUES ('2002-06-08 11:00:00',11.1,6.7,29.83);
INSERT INTO sfo VALUES ('2002-06-08 12:00:00',11.7,6.7,29.84);
INSERT INTO sfo VALUES ('2002-06-08 13:00:00',11.1,6.7,29.85);
INSERT INTO sfo VALUES ('2002-06-08 14:00:00',12.8,6.7,29.85);
INSERT INTO sfo VALUES ('2002-06-08 15:00:00',15,7.2,29.86);
INSERT INTO sfo VALUES ('2002-06-08 16:00:00',17.2,8.9,29.87);
INSERT INTO sfo VALUES ('2002-06-09 00:00:00',18.3,4.4,29.83);
INSERT INTO sfo VALUES ('2002-06-09 01:00:00',16.7,5,29.81);
INSERT INTO sfo VALUES ('2002-06-09 02:00:00',15,5,29.83);
INSERT INTO sfo VALUES ('2002-06-09 04:00:00',11.7,6.7,29.83);
INSERT INTO sfo VALUES ('2002-06-09 05:00:00',11.1,7.2,29.86);
INSERT INTO sfo VALUES ('2002-06-09 06:00:00',11.1,7.2,29.86);
INSERT INTO sfo VALUES ('2002-06-09 07:00:00',11.1,7.2,29.86);
INSERT INTO sfo VALUES ('2002-06-09 08:00:00',11.1,7.2,29.87);
INSERT INTO sfo VALUES ('2002-06-09 09:00:00',11.1,7.2,29.87);
INSERT INTO sfo VALUES ('2002-06-09 10:00:00',11.1,6.7,29.86);
INSERT INTO sfo VALUES ('2002-06-09 11:00:00',11.1,6.7,29.85);
INSERT INTO sfo VALUES ('2002-06-09 12:00:00',11.7,6.1,29.88);
INSERT INTO sfo VALUES ('2002-06-09 13:00:00',16.1,2.2,29.89);
INSERT INTO sfo VALUES ('2002-06-09 14:00:00',17.2,0.6,29.9);
INSERT INTO sfo VALUES ('2002-06-09 15:00:00',16.7,3.3,29.93);
INSERT INTO sfo VALUES ('2002-06-10 00:00:00',23.3,1.7,29.88);
INSERT INTO sfo VALUES ('2002-06-09 23:00:00',23.3,1.1,29.9);
INSERT INTO sfo VALUES ('2002-06-09 22:00:00',22.8,1.1,29.92);
INSERT INTO sfo VALUES ('2002-06-09 21:00:00',21.7,3.3,29.93);
INSERT INTO sfo VALUES ('2002-06-09 18:00:00',20,3.3,29.94);
INSERT INTO sfo VALUES ('2002-06-09 17:00:00',18.9,3.3,29.94);
INSERT INTO sfo VALUES ('2002-06-09 16:00:00',17,4,29.94);
INSERT INTO sfo VALUES ('2002-06-10 01:00:00',22.2,4.4,29.87);
INSERT INTO sfo VALUES ('2002-06-10 02:00:00',21.7,5.6,29.86);
INSERT INTO sfo VALUES ('2002-06-10 03:00:00',19.4,6.7,29.86);
INSERT INTO sfo VALUES ('2002-06-10 04:00:00',18.3,6.7,29.86);
INSERT INTO sfo VALUES ('2002-06-10 05:00:00',17.2,7.8,29.86);
INSERT INTO sfo VALUES ('2002-06-10 06:00:00',17.2,7.2,29.86);
INSERT INTO sfo VALUES ('2002-06-10 07:00:00',17.2,7.2,29.86);
INSERT INTO sfo VALUES ('2002-06-10 08:00:00',16.7,7.2,29.85);
INSERT INTO sfo VALUES ('2002-06-10 11:00:00',17.2,5,29.82);
INSERT INTO sfo VALUES ('2002-06-10 12:00:00',17.8,5,29.83);
INSERT INTO sfo VALUES ('2002-06-10 13:00:00',17.2,5,29.83);
INSERT INTO sfo VALUES ('2002-06-10 14:00:00',20.6,6.1,29.84);
INSERT INTO sfo VALUES ('2002-06-10 15:00:00',19.4,12.2,29.85);
INSERT INTO sfo VALUES ('2002-06-10 16:00:00',21.7,8.9,29.85);
INSERT INTO sfo VALUES ('2002-06-10 17:00:00',22.8,11.1,29.85);
INSERT INTO sfo VALUES ('2002-06-10 18:00:00',24.4,9.4,29.85);
INSERT INTO sfo VALUES ('2002-06-10 19:00:00',24.4,12.8,29.85);
INSERT INTO sfo VALUES ('2002-06-10 20:00:00',26.1,11.1,29.84);
INSERT INTO sfo VALUES ('2002-06-10 21:00:00',27.8,5.6,29.82);
INSERT INTO sfo VALUES ('2002-06-10 22:00:00',27.2,5,29.81);
INSERT INTO sfo VALUES ('2002-06-10 23:00:00',27.2,5.6,29.8);
INSERT INTO sfo VALUES ('2002-06-11 00:00:00',26.7,5.6,29.79);
INSERT INTO sfo VALUES ('2002-06-11 01:00:00',25.6,6.1,29.78);
INSERT INTO sfo VALUES ('2002-06-11 02:00:00',23.3,5.6,29.77);
INSERT INTO sfo VALUES ('2002-06-11 03:00:00',21.7,7.8,29.78);
INSERT INTO sfo VALUES ('2002-06-11 04:00:00',18.3,8.3,29.78);
INSERT INTO sfo VALUES ('2002-06-11 05:00:00',20.6,7.8,29.79);
INSERT INTO sfo VALUES ('2002-06-11 06:00:00',17.8,8.9,29.8);
INSERT INTO sfo VALUES ('2002-06-11 07:00:00',17.2,8.9,29.8);
INSERT INTO sfo VALUES ('2002-06-11 08:00:00',16.1,9.4,29.8);
INSERT INTO sfo VALUES ('2002-06-11 09:00:00',15,9.4,29.8);
INSERT INTO sfo VALUES ('2002-06-11 10:00:00',14.4,9.4,29.79);
INSERT INTO sfo VALUES ('2002-06-11 11:00:00',15,9.4,29.79);
INSERT INTO sfo VALUES ('2002-06-11 12:00:00',15,9.4,29.8);
INSERT INTO sfo VALUES ('2002-06-11 13:00:00',15,8.9,29.81);
INSERT INTO sfo VALUES ('2002-06-11 14:00:00',16.7,8.9,29.82);
INSERT INTO sfo VALUES ('2002-06-11 15:00:00',18.9,11.1,29.84);
INSERT INTO sfo VALUES ('2002-06-11 16:00:00',19.4,13.3,29.85);
INSERT INTO sfo VALUES ('2002-06-11 17:00:00',20.6,13.3,29.85);
INSERT INTO sfo VALUES ('2002-06-11 18:00:00',22.2,14.4,29.86);
INSERT INTO sfo VALUES ('2002-06-11 19:00:00',24.4,11.7,29.86);
INSERT INTO sfo VALUES ('2002-06-11 20:00:00',25,12.2,29.86);
INSERT INTO sfo VALUES ('2002-06-11 21:00:00',26.7,11.7,29.84);
INSERT INTO sfo VALUES ('2002-06-11 22:00:00',25,14.4,29.84);
INSERT INTO sfo VALUES ('2002-06-11 23:00:00',24.4,13.3,29.84);
INSERT INTO sfo VALUES ('2002-06-12 00:00:00',23.9,12.2,29.83);
INSERT INTO sfo VALUES ('2002-06-12 01:00:00',21.1,12.2,29.84);
INSERT INTO sfo VALUES ('2002-06-12 02:00:00',17.2,12.2,29.84);
INSERT INTO sfo VALUES ('2002-06-12 03:00:00',15,11.7,29.87);
INSERT INTO sfo VALUES ('2002-06-12 04:00:00',13.9,11.7,29.88);
INSERT INTO sfo VALUES ('2002-06-12 05:00:00',13.9,11.7,29.89);
INSERT INTO sfo VALUES ('2002-06-12 06:00:00',13.9,11.1,29.9);
INSERT INTO sfo VALUES ('2002-06-12 07:00:00',13.9,11.1,29.92);
INSERT INTO sfo VALUES ('2002-06-12 08:00:00',13.3,11.1,29.92);
INSERT INTO sfo VALUES ('2002-06-12 09:00:00',12.8,11.1,29.91);
INSERT INTO sfo VALUES ('2002-06-12 10:00:00',12.8,11.1,29.91);
INSERT INTO sfo VALUES ('2002-06-12 11:00:00',12.2,10.6,29.91);
INSERT INTO sfo VALUES ('2002-06-12 12:00:00',12.8,10.6,29.92);
INSERT INTO sfo VALUES ('2002-06-12 13:00:00',12.2,10.6,29.92);
INSERT INTO sfo VALUES ('2002-06-12 14:00:00',12.8,10.6,29.93);
INSERT INTO sfo VALUES ('2002-06-12 15:00:00',14.4,11.1,29.94);
INSERT INTO sfo VALUES ('2002-06-12 16:00:00',16.1,11.7,29.95);
INSERT INTO sfo VALUES ('2002-06-12 17:00:00',17.2,11.7,29.96);
INSERT INTO sfo VALUES ('2002-06-12 18:00:00',17.8,12.8,29.97);
INSERT INTO sfo VALUES ('2002-06-12 19:00:00',18.3,12.8,29.97);
INSERT INTO sfo VALUES ('2002-06-12 20:00:00',19.4,12.8,29.97);
INSERT INTO sfo VALUES ('2002-06-12 21:00:00',20,13.3,29.97);
INSERT INTO sfo VALUES ('2002-06-12 22:00:00',22.2,11.7,29.97);
INSERT INTO sfo VALUES ('2002-06-12 23:00:00',19.4,11.7,29.96);
INSERT INTO sfo VALUES ('2002-06-13 00:00:00',16.7,11.7,29.96);
INSERT INTO sfo VALUES ('2002-06-13 01:00:00',16.7,11.7,29.96);
INSERT INTO sfo VALUES ('2002-06-13 02:00:00',15.6,11.1,29.96);
INSERT INTO sfo VALUES ('2002-06-13 03:00:00',14.4,11.1,29.97);
INSERT INTO sfo VALUES ('2002-06-13 04:00:00',13.3,11.1,29.97);
INSERT INTO sfo VALUES ('2002-06-13 05:00:00',13.3,10.6,29.98);
INSERT INTO sfo VALUES ('2002-06-13 06:00:00',12.8,10.6,29.99);
INSERT INTO sfo VALUES ('2002-06-13 07:00:00',12.2,10,30);
INSERT INTO sfo VALUES ('2002-06-13 08:00:00',12.2,10,29.99);
INSERT INTO sfo VALUES ('2002-06-13 09:00:00',11.7,10,29.99);
INSERT INTO sfo VALUES ('2002-06-13 10:00:00',12.2,10,29.98);
INSERT INTO sfo VALUES ('2002-06-13 11:00:00',11.7,10,29.98);
INSERT INTO sfo VALUES ('2002-06-13 12:00:00',11.7,9.4,29.98);
INSERT INTO sfo VALUES ('2002-06-13 13:00:00',11.7,9.4,29.98);
INSERT INTO sfo VALUES ('2002-06-13 14:00:00',11.7,9.4,29.99);
INSERT INTO sfo VALUES ('2002-06-13 15:00:00',11.1,8.9,30.01);
INSERT INTO sfo VALUES ('2002-06-13 16:00:00',11.7,8.9,30.01);
INSERT INTO sfo VALUES ('2002-06-13 17:00:00',13.3,8.9,30.02);
INSERT INTO sfo VALUES ('2002-06-13 18:00:00',13.9,8.9,30.03);
INSERT INTO sfo VALUES ('2002-06-14 00:00:00',16.1,10,30.02);
INSERT INTO sfo VALUES ('2002-06-14 01:00:00',14.4,10,30.02);
INSERT INTO sfo VALUES ('2002-06-14 02:00:00',13.9,10,30.03);
INSERT INTO sfo VALUES ('2002-06-14 03:00:00',13.3,10,30.04);
INSERT INTO sfo VALUES ('2002-06-14 04:00:00',13.3,10,30.04);
INSERT INTO sfo VALUES ('2002-06-14 05:00:00',12.8,10,30.05);
INSERT INTO sfo VALUES ('2002-06-14 06:00:00',12.8,9.4,30.06);
INSERT INTO sfo VALUES ('2002-06-14 07:00:00',12.2,9.4,30.06);
INSERT INTO sfo VALUES ('2002-06-14 08:00:00',12.2,9.4,30.05);
INSERT INTO sfo VALUES ('2002-06-14 09:00:00',12.2,8.9,30.04);
INSERT INTO sfo VALUES ('2002-06-14 10:00:00',11.7,8.9,30.04);
INSERT INTO sfo VALUES ('2002-06-14 11:00:00',11.7,8.9,30.04);
INSERT INTO sfo VALUES ('2002-06-14 12:00:00',11.7,8.3,30.04);
INSERT INTO sfo VALUES ('2002-06-14 13:00:00',11.7,8.3,30.04);
INSERT INTO sfo VALUES ('2002-06-14 14:00:00',11.7,8.9,30.04);
INSERT INTO sfo VALUES ('2002-06-14 15:00:00',11.7,8.3,30.05);
INSERT INTO sfo VALUES ('2002-06-14 16:00:00',12.8,8.3,30.05);
INSERT INTO sfo VALUES ('2002-06-14 17:00:00',13.3,8.9,30.05);
INSERT INTO sfo VALUES ('2002-06-14 18:00:00',14.4,8.9,30.05);
INSERT INTO sfo VALUES ('2002-06-14 19:00:00',15.6,8.9,30.06);
INSERT INTO sfo VALUES ('2002-06-14 20:00:00',16.1,8.9,30.05);
INSERT INTO sfo VALUES ('2002-06-14 22:00:00',16.7,8.3,30.05);
INSERT INTO sfo VALUES ('2002-06-14 23:00:00',16.7,8.3,30.04);
INSERT INTO sfo VALUES ('2002-06-15 00:00:00',16.1,8.3,30.03);
INSERT INTO sfo VALUES ('2002-06-15 01:00:00',15.6,8.3,30.01);
INSERT INTO sfo VALUES ('2002-06-15 02:00:00',15,8.3,30.01);
INSERT INTO sfo VALUES ('2002-06-15 03:00:00',13.9,8.3,30.01);
INSERT INTO sfo VALUES ('2002-06-15 04:00:00',13.3,8.3,30.01);
INSERT INTO sfo VALUES ('2002-06-15 05:00:00',12.8,7.8,30.02);
INSERT INTO sfo VALUES ('2002-06-15 06:00:00',12.8,7.8,30.02);
INSERT INTO sfo VALUES ('2002-06-15 07:00:00',12.2,8.3,30.01);
INSERT INTO sfo VALUES ('2002-06-15 08:00:00',12.2,8.3,30);
INSERT INTO sfo VALUES ('2002-06-15 09:00:00',12.2,8.3,30);
INSERT INTO sfo VALUES ('2002-06-15 10:00:00',11.7,8.3,30);
INSERT INTO sfo VALUES ('2002-06-15 11:00:00',11.1,8.3,29.99);
INSERT INTO sfo VALUES ('2002-06-15 12:00:00',11.1,8.3,30);
INSERT INTO sfo VALUES ('2002-06-15 13:00:00',11.1,8.3,30);
INSERT INTO sfo VALUES ('2002-06-15 14:00:00',11.7,8.9,30.01);
INSERT INTO sfo VALUES ('2002-06-15 15:00:00',12.2,8.9,30.02);
INSERT INTO sfo VALUES ('2002-06-15 16:00:00',13.3,8.9,30.03);
INSERT INTO sfo VALUES ('2002-06-15 17:00:00',14.4,8.9,30.03);
INSERT INTO sfo VALUES ('2002-06-15 18:00:00',15.6,8.9,30.04);
INSERT INTO sfo VALUES ('2002-06-15 19:00:00',16.1,8.9,30.04);
INSERT INTO sfo VALUES ('2002-06-15 20:00:00',17.2,8.9,30.04);
INSERT INTO sfo VALUES ('2002-06-15 21:00:00',17.8,8.9,30.04);
INSERT INTO sfo VALUES ('2002-06-15 22:00:00',17.8,8.9,30.04);
INSERT INTO sfo VALUES ('2002-06-15 23:00:00',17.2,9.4,30.03);
INSERT INTO sfo VALUES ('2002-06-16 00:00:00',16.1,8.9,30.03);
INSERT INTO sfo VALUES ('2002-06-16 01:00:00',15.6,8.9,30.02);
INSERT INTO sfo VALUES ('2002-06-16 02:00:00',15,8.9,30.02);
INSERT INTO sfo VALUES ('2002-06-16 03:00:00',13.9,8.9,30.03);
INSERT INTO sfo VALUES ('2002-06-16 04:00:00',13.3,8.9,30.03);
INSERT INTO sfo VALUES ('2002-06-16 05:00:00',13.3,8.3,30.05);
INSERT INTO sfo VALUES ('2002-06-16 06:00:00',12.8,8.3,30.05);
INSERT INTO sfo VALUES ('2002-06-16 07:00:00',12.8,8.3,30.06);
INSERT INTO sfo VALUES ('2002-06-16 08:00:00',12.2,8.3,30.05);
INSERT INTO sfo VALUES ('2002-06-16 09:00:00',11.7,8.3,30.04);
INSERT INTO sfo VALUES ('2002-06-16 10:00:00',11.7,8.3,30.04);
INSERT INTO sfo VALUES ('2002-06-16 11:00:00',11.7,8.3,30.04);
INSERT INTO sfo VALUES ('2002-06-16 12:00:00',11.7,8.3,30.05);
INSERT INTO sfo VALUES ('2002-06-16 13:00:00',11.7,8.9,30.06);
INSERT INTO sfo VALUES ('2002-06-16 14:00:00',12.2,9.4,30.08);
INSERT INTO sfo VALUES ('2002-06-16 15:00:00',13.9,9.4,30.09);
INSERT INTO sfo VALUES ('2002-06-16 16:00:00',15,9.4,30.1);
INSERT INTO sfo VALUES ('2002-06-16 17:00:00',15.6,9.4,30.1);
INSERT INTO sfo VALUES ('2002-06-16 19:00:00',17.8,8.9,30.11);
INSERT INTO sfo VALUES ('2002-06-16 18:00:00',16.7,8.9,30.11);
INSERT INTO sfo VALUES ('2002-06-16 20:00:00',18.9,9.4,30.1);
INSERT INTO sfo VALUES ('2002-06-16 21:00:00',18.9,9.4,30.1);
INSERT INTO sfo VALUES ('2002-06-16 22:00:00',18.3,10,30.09);
INSERT INTO sfo VALUES ('2002-06-16 23:00:00',18.3,10,30.07);
INSERT INTO sfo VALUES ('2002-06-17 00:00:00',17.8,10,30.05);
INSERT INTO sfo VALUES ('2002-06-17 01:00:00',17.2,10,30.05);
INSERT INTO sfo VALUES ('2002-06-17 02:00:00',16.1,9.4,30.04);
INSERT INTO sfo VALUES ('2002-06-17 03:00:00',15,9.4,30.04);
INSERT INTO sfo VALUES ('2002-06-17 04:00:00',14.4,9.4,30.04);
INSERT INTO sfo VALUES ('2002-06-17 05:00:00',14.4,9.4,30.06);
INSERT INTO sfo VALUES ('2002-06-17 06:00:00',13.9,8.9,30.07);
INSERT INTO sfo VALUES ('2002-06-17 07:00:00',13.3,8.9,30.07);
INSERT INTO sfo VALUES ('2002-06-17 08:00:00',12.8,9.4,30.06);
INSERT INTO sfo VALUES ('2002-06-17 09:00:00',12.8,9.4,30.05);
INSERT INTO sfo VALUES ('2002-06-17 10:00:00',12.8,9.4,30.05);
INSERT INTO sfo VALUES ('2002-06-17 11:00:00',12.2,9.4,30.06);
INSERT INTO sfo VALUES ('2002-06-17 12:00:00',12.2,9.4,30.06);
INSERT INTO sfo VALUES ('2002-06-17 13:00:00',12.2,9.4,30.06);
INSERT INTO sfo VALUES ('2002-06-17 14:00:00',12.2,10,30.07);
INSERT INTO sfo VALUES ('2002-06-17 15:00:00',11.7,10,30.09);
INSERT INTO sfo VALUES ('2002-06-17 16:00:00',13.3,10,30.09);
INSERT INTO sfo VALUES ('2002-06-17 17:00:00',15,10,30.09);
INSERT INTO sfo VALUES ('2002-06-17 18:00:00',16.1,10,30.08);
INSERT INTO sfo VALUES ('2002-06-17 19:00:00',17.2,9.4,30.09);
INSERT INTO sfo VALUES ('2002-06-17 20:00:00',19.4,8.3,30.07);
INSERT INTO sfo VALUES ('2002-06-17 21:00:00',19.4,8.9,30.06);
INSERT INTO sfo VALUES ('2002-06-17 22:00:00',20,8.9,30.04);
INSERT INTO sfo VALUES ('2002-06-17 23:00:00',20.6,9.4,30.02);
INSERT INTO sfo VALUES ('2002-06-18 00:00:00',20,8.9,30.02);
INSERT INTO sfo VALUES ('2002-06-18 01:00:00',18.3,10.6,30);
INSERT INTO sfo VALUES ('2002-06-18 02:00:00',17.2,10.6,30.01);
INSERT INTO sfo VALUES ('2002-06-18 03:00:00',16.7,11.7,30.02);
INSERT INTO sfo VALUES ('2002-06-18 04:00:00',16.1,12.2,30.02);
INSERT INTO sfo VALUES ('2002-06-18 05:00:00',15.6,12.2,30.03);
INSERT INTO sfo VALUES ('2002-06-18 06:00:00',14.4,12.2,30.03);
INSERT INTO sfo VALUES ('2002-06-18 07:00:00',14.4,12.2,30.03);
INSERT INTO sfo VALUES ('2002-06-18 08:00:00',13.9,12.2,30.03);
INSERT INTO sfo VALUES ('2002-06-18 09:00:00',13.3,12.2,30.02);
INSERT INTO sfo VALUES ('2002-06-18 10:00:00',13.9,12.8,30.01);
INSERT INTO sfo VALUES ('2002-06-18 11:00:00',13.9,13.3,30.01);
INSERT INTO sfo VALUES ('2002-06-18 12:00:00',NULL,NULL,30.02);
INSERT INTO sfo VALUES ('2002-06-18 13:00:00',NULL,NULL,30.03);
INSERT INTO sfo VALUES ('2002-06-18 12:56:00',14,13,30.03);
INSERT INTO sfo VALUES ('2002-06-18 14:00:00',14.4,12.8,30.03);
INSERT INTO sfo VALUES ('2002-06-18 15:00:00',15,13.9,30.04);
INSERT INTO sfo VALUES ('2002-06-18 16:00:00',15.6,13.3,30.04);
INSERT INTO sfo VALUES ('2002-06-18 17:00:00',15.6,12.2,30.04);
