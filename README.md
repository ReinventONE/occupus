## Potty Occupancy Observation Casting Platform

*Potty Occupancy Observational Cast* (PooCast) – is an Arduino-based restroom occupancy detection and rapid notification system based on a network of nRF24L01+ 2.4GHz RF radios. It supports multiple (up to 5) observer units installed typically in each toilet, and a single display unit – used to monitor the occupancy status remotely.  The Display unit can be implemented in a variety of ways, your imagination is the limit. In this version display is implemented via 2 sets of 8x8 LED Matrices, as well as via serving the status data as JSON over HTTP via the Ethernet Arduino Shield.

This project came out of an actual need to be aware when one of two bathrooms at the new office is occupied. Understanding this status saves people valuable time at work, and allows creative types to stay in the flow until someone, finally, eventually, decides to flush that damn toilet and get the hell out.  No more guessing – which bathroom is free? Is it the one upstairs, or on the 2nd floor?  What about around the corner?

Simply install one of the sensor modules in each bathroom, aiming at the toilet, adjust the Sonar distance parameters, repeat for each other bathroom, and then install the display unit in a common well visible place.  Optionally, write a web app that pulls JSON http server, and provides you with a rolling average of your office toilet time :)

Below is the picture of all components participating in this function:

![PooCast Overview](images/PooCast-Overview.jpg)

## Observer/Sensor Module

I created two modules, which are very similar, but the second unit has more functionality.

### Mechanism

Observer module uses three sensors to decide if the bathroom is occupied or not:

 * Light sensor
 * IR Motion sensor
 * Distance sensor (ultrasound)

Logic is something like this: if the light is off, nobody is there.  If the light is on, let's look at Motion data: if someone was detected within a few seconds, consider occupied. If not, check distance sensor (presumably pointed at the toilet itself) and if it detects distance below configured threshold, someone is sitting on the toilet.  Obviously all thresholds are environment specific.

Observer unit transmits it's status to the Display unit via [RF24 wireless module](http://maniacbug.wordpress.com/2011/11/02/getting-started-rf24/).

Next are some pictures of the sensor modules.

#### Early Sensor Module

![Sensor Module #1](images/PooCast-EarlySensor.jpg)

#### Advanced Sensor Module

![Sensor Module #2](images/PooCast-NewerSensor.jpg)

This sensor unit has an additional rotary knob (which can be clicked, for a button effect).  I used the knob to provide optional real time configuration interface, which saves all changes to EEPROM.

## Display Module

Primary way the display unit informs users is via two sets of LED Matrices, shown below.

![LED Matrices](images/PooCast-Display-Unit.jpg)

Each is driven by a Rainbowduino, and a serial connection is used from Arduino UNO, to the first Rainboduino, then to the second one.

### TCP/IP

Reporter module has ability to provide status over the TCP/IP network. Using Ethernet Arduino Shield
we are able to run a small HTTP server that serves JSON.

![Display Module JSON Response](images/PooCast-JSON.jpg)

### Curl Session

```bash
→ curl -v http://172.16.0.90/ --header 'Content-Type: application/json' --header 'Accept: application/json'

> GET / HTTP/1.1
> User-Agent: curl/7.30.0
> Host: 172.16.0.90
> Content-Type: application/json
> Accept: application/json
>
< HTTP/1.1 200 OK
< Content-Type: application/json
< Accept: application/json
< Connection: close
< Refresh: 3
<
[{
	"id": 1,
	"connected": true,
	"status": false
}, {
	"id": 2,
	"connected": false
}]
```

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Added some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

## Author

Konstantin Gredeskoul, @kig, http://github.com/kigster
