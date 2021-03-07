# Roba sul signaling WebRTC

## Codec

Opus per l'audio, MIME `audio/ogg; codecs=vorbis`

## Appunti

Bisogna scambiarsi gli ICE candidates, che definiscono come i peer comunicheranno tra loro. Lo streaming degli ICE è continuo; man mano che un peer trova possibili parametri li invia, anche dopo l'inizio della chiamata. I due peer continuano a ricevere ICE segnalati fino a quando non ne trovano uno che sia supportato da entrambi, viene estratto l'SDP dal candidato e viene usato da loro per instaurare la connessione.

L'ICE è definito come un JSON contenente tre campi, `type` che sarà sempre `new-ice-candidate`, `target` che indica l'username dell'utente al quale voglio inviare il candidato, e `candidate`, che contiene l'SDP, che va semplicemente sparato al target.

### Implementazione

Visto che quando un client si connette non può inviare nulla nel body del Websocket upgrade, posso dare un ID arbitrario ad ogni connessione. Dopodiché dovrò chiedere al client di darmi il suo username, oppure potrei prendermelo dalla prima richiesta che il client mi fa, del tipo che il client quando vuole segnalare invia il suo nome, il target e il resto dei dati.
