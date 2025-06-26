# Sprawozdanie z projektu – Symulacja działania windy, Mateusz Dadura i Karol Betlejewski, ACiR

## Wstęp

W ramach projektu stworzyliśmy prostą aplikację symulującą działanie windy, wykorzystując bibliotekę GDI+ w języku C++. Głównym celem było odwzorowanie zachowania windy poruszającej się pomiędzy piętrami budynku i reagującej na przyciski wywołujące pasażerów, i kierujące windę na kolejne piętra.

Całość została zaimplementowana w jednym pliku źródłowym, z podziałem na kilka klas opisujących poszczególne elementy symulacji.

---

## Struktura kodu

Kod składa się z trzech głównych klas:

### 1. `Button`

Reprezentuje pojedynczy przycisk, który może być klikany przez użytkownika.

**Najważniejsze właściwości:**
- `shape` (prostokąt – kształt graficzny przycisku),
- `text` (etykieta na przycisku, np. numer piętra),
- `floor` (piętro przypisane do danego przycisku),
- `active` (czy dany przycisk został aktywowany),
- `isInternal` (czy przycisk znajduje się wewnątrz windy).

**Funkcjonalności:**
- Rysowanie przycisku na ekranie.
- Obsługa kliknięcia – przycisk staje się aktywny po kliknięciu myszką.
- Resetowanie aktywności przycisku.

---

### 2. `Elevator`

To serce symulacji – cała logika działania windy znajduje się właśnie tutaj.

**Najważniejsze właściwości:**
- `currentFloor`, `targetFloor`, `y` – aktualna pozycja windy i cel podróży.
- `speed`, `height`, `width` – parametry poruszania się i wyglądu windy.
- `shape` – graficzna reprezentacja kabiny.
- `isMoving` – flaga informująca o tym, czy winda właśnie jedzie.
- `internalButtons` – "życzenia" pasażerów.

**Funkcjonalności:**
- Ruch windy między piętrami – obliczany na podstawie różnicy między aktualną a docelową pozycją.
- Przemieszczanie windy w pionie (animacja).
- Wyznaczanie nowego celu podróży (nowe piętro).
- Obsługa "życzeń" pasażerów.

---

### 3. `ElevatorSystem`

To klasa zarządzająca całością – zawiera windę i przyciski zewnętrzne.

**Najważniejsze właściwości:**
- `elevator` – instancja windy.
- `externalButtons` – przyciski wywołujące pasażerów z każdego piętra.

**Funkcjonalności:**
- Przekazywanie kliknięć do odpowiednich przycisków.
- Przekazywanie poleceń do windy (czyli reagowanie na kliknięcia).
- Rysowanie wszystkiego – windy i wszystkich przycisków.

---

## Działanie programu

Aplikacje tworzymy za pomocą kompilacji poprzez komende wpisywaną w terminalu:

`g++ main.cpp -o windasim.exe -lgdiplus -mwindows`

Po uruchomieniu aplikacji otwiera się okno (400x600 px), w którym widzimy windę oraz zestaw przycisków służących do inicjowania na danym piętrze pasażerów z przypisanym piętrem, na które chcą się dostać.

Winda może być w ruchu tylko w jednym kierunku w danym czasie. Gdy dotrze na żądane piętro, przycisk przestaje być aktywny. Podczas jazdy można kolejkować kolejnych pasażerów. Po zakończeniu jednej akcji, winda zacznie wykonywać kolejną. Jest możliwe zabieranie pasażerów odpowiadających innym akcjom, jeśli winda ma po drodze dane piętro. Maksymalny udźwig windy to 700kg, co odpowiada 10 pasażerom.

---

## Podsumowanie

Projekt jest funkcjonalnym prototypem prostego systemu windy. Dobrze odzwierciedla podstawowe zasady działania wind: reakcja na wezwania, poruszanie się między piętrami oraz wizualizacja stanu. Użycie wątków umożliwia jednoczesne działanie windy i obsługę interfejsu graficznego.
