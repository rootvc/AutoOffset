# AutoOffset

AutoOffset uses a [Particle Photon](https://store.particle.io/products/photon) and [Carloop OBDII](https://www.carloop.io/) adapter to automatically offset carbon emissions from vehicles using the [Wren.co](https://wren.co) API.

This code integrates speed readouts fom [OBDII](https://x-engineer.org/on-board-diagnostics-obd-modes-operation-diagnostic-services/)—while some cars have fuel level PIDs, it is not standard across vehicles and years the way speed PIDs are and I wanted to make this as compatible with as many vehicles as possible.

This app could also be implemented with [GPS](https://store.particle.io/collections/particle-tracking-system-1) and cellular instead of OBDII readouts and WiFi, but I wanted to make an app that didn't require tracking or persistent data. Let me know if you make the more sophisticated version!

## Getting Started

### Dependencies

* [Particle.io Photon](https://store.particle.io/products/photon)
* [Carloop](https://www.carloop.io/) (you may have to find one used)
* [Wren.co](https://www.wren.co/wren-api) API key
* A vehicle with [OBDII](http://www.obdii.com/connector.html)

### Installing

* [Setup](https://docs.particle.io/quickstart/photon/) the Particle Photon and flash it with AutoOffset.ino. Be sure to include the Carloop library using the Particle Web IDE or [directly](https://github.com/carloop/carloop-library).
* Register a [Wren.co](https://www.wren.co/wren-api) API key
* Setup a [Particle Webhook] titled (https://console.particle.io/) ```Offset Carbon``` to ```POST``` to https://www.wren.co/api/offset-orders with your Wren API key in the headers and ```CUSTOM BODY``` set to:
```
{"tons":{{{PARTICLE_EVENT_VALUE}}},"note":"tons carbon offset","dryRun":true}
```
* Configure the MPG variable in the source code to match your cars

### Executing program

* With the Photon setup, install it in your [car's OBDII port](https://www.fixdapp.com/blog/where-is-my-obd2-port/#:~:text=The%20most%20common%20place%20to,passenger's%20side%20of%20the%20car.)
* The app should run automatically and publish to the Particle Webhook when the Particle connects to internet in test mode (ie does not charge your Wren account)
* When you are done testing, the ```dryRun``` flag in Particle Webhook body can be set to ```false``` to start buying offsets.

## Help

Any advise for common problems or issues.
```
command to run if program contains helper info
```

## Authors

Contributors names and contact info

ex. Dominique Pizzie  
ex. [@DomPizzie](https://twitter.com/dompizzie)

## Version History

* 0.2
    * Various bug fixes and optimizations
    * See [commit change]() or See [release history]()
* 0.1
    * Initial Release

## License

This project is licensed under the [NAME HERE] License - see the LICENSE.md file for details

## Acknowledgments

Inspiration, code snippets, etc.
* [awesome-readme](https://github.com/matiassingers/awesome-readme)
* [PurpleBooth](https://gist.github.com/PurpleBooth/109311bb0361f32d87a2)
* [dbader](https://github.com/dbader/readme-template)
* [zenorocha](https://gist.github.com/zenorocha/4526327)
* [fvcproductions](https://gist.github.com/fvcproductions/1bfc2d4aecb01a834b46)