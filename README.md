# AutoOffset

AutoOffset uses a [Particle Photon](https://store.particle.io/products/photon) and [Carloop OBDII](https://www.carloop.io/) adapter to automatically offset carbon emissions from vehicles using the [Wren.co](https://wren.co) API.

## Getting Started

### Dependencies

* [Particle.io Photon](https://store.particle.io/products/photon)
* [Carloop](https://www.carloop.io/) (you may have to find one used)
* [Wren.co](https://www.wren.co/wren-api) API key
* A vehicle with [OBDII](http://www.obdii.com/connector.html)

### Installing

* [Setup](https://docs.particle.io/quickstart/photon/) the Particle Photon and flash it with AutoOffset.ino
* Register a [Wren.co](https://www.wren.co/wren-api) API key
* Setup a [Particle Webhook](https://console.particle.io/) ```Offset Carbon``` to ```POST``` to https://www.wren.co/api/offset-orders with ```CUSTOM BODY```
```
{"tons":{{{PARTICLE_EVENT_VALUE}}},"note":"total tons carbon offset","dryRun":true}
```

### Executing program

* How to run the program
* Step-by-step bullets
```
code blocks for commands
```

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