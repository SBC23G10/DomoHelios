# _SBC23G10 "DomoHelios"_

Proyecto propuesto en la asignatura Sistemas Basados en Computador de la ETSI de Sistemas Informaticos de la UPM del año 2023 (grupo 10).

## DomoHelios ¿Qué es?

Se trata de un intento de sistema de gestión del hogar (domótica) y carga energética retroalimentado a partir de energía solar.

## Implementaciones clave

El proyecto consta de los siguientes elementos clave:
- Control de orientación del panel en tiempo real (esta limitado a plazos para ahorro energético).
- Carga simultánea de bateria y control de su consumo
- Proporcionar la funcionalidad básica domótica, control de actuadores
- Persistencia y consulta de datos bajo demanda

Su funcionamiento puede ser facilmente extendido a un control de N paneles solares a escala industrial (etherCAT).

## Hardware empleado

### Elementos generales

Se hace uso del microcontrolador ESP-32 de Espressif Systems, placa NodeMCU.

### Elementos específicos

Entre otros, principalmente:
- 1x Panel solar de 180 x 140 mm
- Nx Bateria
- 1x Controlador bateria(s) (el modelo depende de la composición de estas)
- 2x Servomotor MG996R (inicialmente SG90 reemplazado por falta de torque)
- 1x Caja de empalme redonda (base rotor)
- 1x Plancha de PVC (recortes soporte panel, base y casa)
- 1x Bomba de agua 3v 4546 adafruit (Actuador "Bomba")
- 1x Led naranja (Actuador "Iluminación")
- 2x Transistor Darlington "tip120" (Apertura a tierra)
- 4x escuadras metalicas (horquilla de sujeción)

## ¿Qué librerias utiliza?

- Hace uso del SDK de control encapsulado MQTT & RPC del repositorio de ThingsBoard que puedes encontrar en [thingsboard-client-sdk](https://github.com/thingsboard/thingsboard-client-sdk).

- Para el tratamiento/generacion de ficheros JSON se hace uso de la libreria cJSON de [DaveGamble](https://github.com/DaveGamble) que puedes encontrar en [cJSON](https://github.com/DaveGamble/cJSON),

    asi como la libreria de rápida serializacion basada en operador[] de C++ de [bblanchon](https://github.com/bblanchon) que puedes encontrar en [ArduinoJSON](https://github.com/bblanchon/ArduinoJson)

- Se ha desarrollado una libreria de propósito general de fácil control de SERVOMOTORES en C++ de precision en coma flotante (precisión simple) basándose en la multiplicidad de terminos decimales y POSTERIOR redondeo del valor de carga util (duty cycle) (para el control del panel en 360.00 grados completos) que puedes encontrar en [esp-servo-util](https://github.com/SBC23G10/esp_servo_util).

    Puede ser facilmente portado para cualquier sistema de multiples servomotores sobre abstracciones que funcionen a través de etherCAT.

Además de librerias de implementación abstractas de uso específico (desarrolladas específicamente para el proyecto).

## Galería de testeo y elaboración del proyecto

Pruebas del funcionamiento "medianamente fluido" [en modo tiempo real] de la estimulación por luz ldr en forma de rotación de los servomotores, limitaciones por ser los ejes no los ideales (falta de lubricación, etc). Se podría imprimir en 3D un soporte o soportes eje base y horquilla de sujeción (Posibles futuras versiones)

<p align="center">
  <img height=600 src="https://raw.githubusercontent.com/SBC23G10/DomoHelios/main/showoff/solar_kinect.gif" alt="animated" />
</p>

Pruebas de control de carga y monitorización de su voltaje.