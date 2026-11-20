# TesisUE

Proyecto Unreal Engine 5.6. C++ en `Source/TesisUE`, assets en `Content/` (los `.uasset` son binarios).

## Delegación de agentes

La sesión principal orquesta y delega con la herramienta Agent:

- `reader` (Haiku): para leer más de ~3 archivos o ubicar dónde vive algo. Pide siempre un resumen corto.
- `planner` (Opus; volver a Fable 5.2 cuando haya créditos): para tareas grandes o ambiguas, antes de tocar código. Devuelve un plan por pasos.
- `coder` (Sonnet): para implementar un plan o brief concreto.

Reglas:
- Tarea chica (1-2 archivos conocidos): hazla directo, sin delegar.
- Cada brief a un agente debe incluir objetivo, archivos, restricciones y criterio de "hecho"; los agentes no ven esta conversación.
- No des por buena la entrega de `coder`: revisa el diff y el resultado de la compilación antes de reportar.
- Dos `coder` en paralelo nunca tocan los mismos archivos.

## Código

- No escribir comentarios en el código fuente (`Source/`). El código debe ser autoexplicativo mediante nombres claros.
