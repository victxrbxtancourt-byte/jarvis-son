# Etapa 2: Captura y normalización de audio

## Comando reproducible

```bash
./scripts/capture.sh 6 prueba.wav
```

## Resultado de la prueba

Verificación del formato con `ffprobe`:

```
Input #0, wav, from 'prueba.wav':
  Duration: 00:00:06.14, bitrate: 256 kb/s
  Stream #0:0: Audio: pcm_s16le, 16000 Hz, 1 channels, s16, 256 kb/s
```

- Formato: WAV, PCM 16 bits, 16 kHz, mono.
- El clip se reprodujo correctamente con `termux-media-player`.
- Al terminar no quedaron archivos temporales en `$TMPDIR`.

## Pruebas de validación

| Comando | Resultado esperado | Resultado obtenido |
|---|---|---|
| `./scripts/capture.sh abc` | Error: no es un número | `ERROR: los segundos deben ser un número entero` |
| `./scripts/capture.sh 99` | Error: fuera de rango | `ERROR: usa entre 1 y 30 segundos` |

## Qué hace el script

- Graba un clip corto iniciado por el usuario con Termux:API.
- Lo convierte a WAV mono, 16 kHz, PCM 16 bits con FFmpeg.
- Valida formato y duración (mínimo 0,5 s); si falla, no deja un archivo inválido.
- Elimina los temporales siempre, incluso si falla (`trap`).
- No construye comandos de shell con texto externo; los argumentos se validan.

## Privacidad

- Los `.wav` están en `.gitignore` y no se suben al repositorio.
- El clip de prueba contenía solo una frase neutra.

## Limitaciones

- Duración limitada a 1–30 s.
- La calidad depende del micrófono del teléfono y del ruido ambiente.
- La grabación usa una espera fija (`sleep`), no detecta el fin del habla.