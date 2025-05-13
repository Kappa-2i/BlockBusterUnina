# **BlockBusterUnina**

## **Introduzione**
 *BlockBuster Unina* è una piattaforma client-server per la gestione di una videoteca, progettata per offrire agli utenti la possibilità di registrarsi, cercare, noleggiare e restituire film.

## **Funzionalità**
### **Autenticazione**
<ins>**Registrazione**</ins>: REGISTRAZIONE <username\> <password\>

<ins>**Login**</ins>: LOGIN <username\> <password\>

### **Catalogo Film**
<ins>**Ricerca film**</ins>: CERCA <titolo\>

### **Carrello**
<ins>**Aggiungi al carrello**</ins>: AGGIUNGI_AL_CARRELLO <titolo\>

<ins>**Rimuovi dal carrello**</ins>: RIMUOVI_DAL_CARRELLO <titolo\>

<ins>**Visualizza carrello**</ins>: VISUALIZZA_CARRELLO

### **Noleggio**
<ins>**Checkout**</ins>: CHECKOUT (conferma noleggio dei film nel carrello)

<ins>**Restituzione film**</ins>: RESTITUISCI_FILM <titolo\>

<ins>**Visualizza prestiti attivi**</ins>: VISUALIZZA_PRESTITI

### **Notifiche**
<ins>**Messaggi di notifica**</ins>: VISUALIZZA_MESSAGGI (mostra film con prestiti scaduti)

### **Limitazioni**
**Numero massimo di film noleggiabili contemporaneamente**: 10

**Durata del noleggio**: 7 giorni

<ins>*Il sistema gestisce automaticamente i prestiti scaduti*</ins>


## **Configurazione con Docker**
### **Prerequisiti**
1. Docker installato

2. Docker Compose installato

### **Avvio dell'applicazione**

1. Posizionarsi nella directory del progetto

2. Eseguire il comando:
```bash
    docker-compose up --build
```
### **Avvio del client**

1. Eseguire il comando:
```bash
    docker-compose exec -it blockbusterclient-unina-1
```

### **Comandi utili**
<ins>**Arrestare i container**</ins>: `docker-compose down`

<ins>**Visualizzare i log**</ins>: `docker-compose logs`
 
<ins>**Riavviare un servizio specifico**</ins>: `docker-compose up --build <nome_servizio>`

<ins>**Visualizzare i container in esecuzione**</ins>: `docker ps`