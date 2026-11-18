# Práctica 4 — Procesamiento, Almacenamiento y Lógica de Negocio de Objetos Inteligentes con AWS y Alexa

**SmartPark — Integración de un objeto inteligente con AWS (IoT Core, IoT Rules, Lambda y DynamoDB) y Alexa, con procesamiento, almacenamiento y lógica de control en el Edge y/o en la Nube**

**Carrera:** Ingeniería de Sistemas

**Docente:** Eduardo Enrique Marin Garcia

**Asignatura:** Internet de las Cosas [SIS-234]

**Semestre:** 2-2026

**Grupo:** 9

**Integrantes:**

- Carlos Eduardo Iriarte Rodriguez
- Isabella Catherine Montes Iñiguez
- Henry Alejandro Rivera Mendez

---

## Índice

1. [Caso de Uso y Requerimientos](#1-caso-de-uso-y-requerimientos)
2. [Análisis y Diseño](#2-análisis-y-diseño)
3. [Desarrollo e Implementación](#3-desarrollo-e-implementación)
4. [Pruebas y Validaciones](#4-pruebas-y-validaciones)
5. [Resultados](#5-resultados)
6. [Conclusiones](#6-conclusiones)
7. [Recomendaciones](#7-recomendaciones)
8. [Anexos](#8-anexos)

---

## 1. Caso de Uso y Requerimientos

### 1.1 Caso de uso y ampliación respecto a la Práctica 3


### 1.2 Justificación de los sensores y actuadores


### 1.3 Requerimientos funcionales


### 1.4 Requerimientos no funcionales


### 1.5 Trazabilidad de los requerimientos


---

## 2. Análisis y Diseño

### 2.1 Diagrama de bloques del objeto inteligente


### 2.2 Diagrama de arquitectura


### 2.3 Diagrama de circuito


### 2.4 Diagramas de clases

#### 2.4.1 Objeto inteligente (firmware ESP32)


#### 2.4.2 Funciones Lambda


### 2.5 Diagramas de secuencia

#### 2.5.1 Consulta del sensor y comando sobre el actuador


#### 2.5.2 Almacenamiento de un dato (objeto → regla de IoT → Lambda → DynamoDB)


#### 2.5.3 Ejecución de la regla automática en el Edge


#### 2.5.4 Ejecución de las reglas automáticas en la Nube


### 2.6 Lógica de control (diagrama de estados y ubicación Edge / Nube)


### 2.7 Especificación del documento Shadow


### 2.8 Modelo de datos de DynamoDB


### 2.9 Reglas de AWS IoT y funciones Lambda


### 2.10 Política de reporte


### 2.11 Diseño de la skill de Alexa


### 2.12 Diseño de reportes (mockups)


---

## 3. Desarrollo e Implementación

### 3.1 Estructura del código


### 3.2 Configuraciones en AWS y permisos asignados


### 3.3 Configuraciones en Alexa


---

## 4. Pruebas y Validaciones

### 4.1 Plan de pruebas y hojas de registro


### 4.2 Pruebas funcionales


### 4.3 Pruebas no funcionales


### 4.4 Análisis de fallas y correcciones


---

## 5. Resultados

### 5.1 Resumen de cumplimiento


### 5.2 Resultados de las pruebas no funcionales


### 5.3 Datos almacenados en DynamoDB


### 5.4 Análisis de la decisión Edge / Nube


### 5.5 Desviaciones y observaciones


---

## 6. Conclusiones



---

## 7. Recomendaciones


---

## 8. Anexos

### 8.1 Glosario


### 8.2 Referencias


### 8.3 Evidencias de las pruebas


### 8.4 Evidencias de arquitectura y configuración

