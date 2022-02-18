# AutoOffset

AutoOffset uses a [Particle Photon](https://store.particle.io/products/photon) and [Carloop OBDII](https://www.carloop.io/) adapter to automatically offset carbon emissions from vehicles using the [Wren.co](https://wren.co) API.

This code integrates speed readouts fom [OBDII](https://x-engineer.org/on-board-diagnostics-obd-modes-operation-diagnostic-services/) to calculate distance and emissions. While some cars have fuel level PIDs, it is not standard across vehicles and years the way speed PIDs are and I wanted to make this as compatible with as many vehicles as possible.

This app could also be implemented with [GPS](https://store.particle.io/collections/particle-tracking-system-1) and cellular instead of OBDII readouts and WiFi, but I wanted to make an app that didn't require tracking or persistent data. Let me know if you make the more sophisticated version!

## Getting Started

### Dependencies

* [Particle.io Photon](https://store.particle.io/products/photon)
* [Carloop](https://www.carloop.io/) (you may have to find one used)
* [Wren.co](https://www.wren.co/wren-api) API key
* A vehicle with [OBDII](http://www.obdii.com/connector.html)

### Installing

* [Setup](https://docs.particle.io/quickstart/photon/) the Particle Photon and flash it with [AutoOffset.ino](https://github.com/kanetronv2/AutoOffset/blob/main/autooffset.ino). Be sure to include the Carloop library using the Particle Web IDE or [directly](https://github.com/carloop/carloop-library).
* Register a [Wren.co](https://www.wren.co/wren-api) API key
* Setup a [Particle Webhook](https://console.particle.io/) titled ```Offset Carbon``` to ```POST``` to https://www.wren.co/api/offset-orders with your Wren API key in the headers and ```CUSTOM BODY``` set to:
```
{"tons":{{{PARTICLE_EVENT_VALUE}}},"note":"tons carbon offset","dryRun":true}
```
* Configure the MPG variable in the source code to match your cars (line 17 in [AutoOffset.ino](https://github.com/kanetronv2/AutoOffset/blob/main/autooffset.ino))

### Executing program

* With the Photon setup, install it in your [car's OBDII port](https://www.fixdapp.com/blog/where-is-my-obd2-port/#:~:text=The%20most%20common%20place%20to,passenger's%20side%20of%20the%20car.)
* The app should run automatically and publish to the Particle Webhook when the Particle connects to internet in test mode (ie does not charge your Wren account)
* When you are done testing, the ```dryRun``` flag in Particle Webhook body can be set to ```false``` to start buying offsets.

## Help

Please help me refine this project. What I'd like to implement:

* Deep sleep the Particle when car is off
* Offset over cellular

## Authors

[@kane](https://twitter.com/kane)

## License

This project is licensed under the Creative Commons Zero v1.0 Universal License - see the LICENSE.md file for details

## Acknowledgments

Thanks for the help:

* [@jenlwei](https://twitter.com/jenlwei) - testing
* [@bstnfld](https://twitter.com/bstnfld) - co-founder, Wren.co
* [@mondalan](https://twitter.com/mondalan) - founder, Carloop.io
* [@zs](https://twitter.com/zs) - co-founder, Particle.io