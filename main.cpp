// ODPALANIE: g++ main.cpp -o windasim.exe -lgdiplus -mwindows

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace Gdiplus;

ULONG_PTR gdiplusToken;

constexpr int FLOORS = 5;
constexpr float verticalOffset = 80.0f;
constexpr int capacity = 10;
constexpr int PASSENGER_WEIGHT = 70;

enum PersonState {
    Waiting,    
    Walking,    
    InElevator, 
    Exiting     
};

struct Person {
    int fromFloor;
    int toFloor;
    float x, y;
    PersonState state;
    bool entered;
    bool justEnteredThisStop;
};

#define TIMER_ID 1
#define TIMER_INTERVAL 30

Image* personImage = nullptr;

class Elevator
{
public:
    float position;
    int currentFloor;
    bool movingUp;
    bool moving;
    bool boarding;
    DWORD boardingStartTick = 0;

    std::vector<int> waitingPeople;
    int passengers;

    int waitingPeopleDest[FLOORS][FLOORS] = { 0 };
    int onboardDest[FLOORS] = {0};

    std::vector<Person> people;

    Elevator()
        : position(-1), currentFloor(0), movingUp(true), moving(false), boarding(false),
          waitingPeople(FLOORS, 0), passengers(0)
    {
        std::fill(onboardDest, onboardDest + FLOORS, 0);
    }

    void call(int from, int to, int clientWidth, int clientHeight) {
        if (from >= 0 && from < FLOORS && to >= 0 && to < FLOORS && from != to) {
            waitingPeopleDest[from][to]++;
            waitingPeople[from]++;
            updateMovement();

            Person p;
            p.fromFloor = from;
            p.toFloor = to;
            p.state = Waiting;
            p.entered = false;
            p.justEnteredThisStop = false;
            float usableHeight = clientHeight - 100;
            float floorHeight = usableHeight / FLOORS;
            p.x = 60.0f;
            p.y = verticalOffset + 50 + (FLOORS - 1 - from) * floorHeight - 30;
            people.push_back(p);
        }
    }

    void setPositionByFloor(int floor, int clientHeight)
    {
        float usableHeight = clientHeight - 100;
        float floorHeight = usableHeight / FLOORS;
        position = floorToPos(floor, clientHeight, floorHeight / 2);
        currentFloor = floor;
    }

    static float floorToPos(int floor, int clientHeight, float elevatorHeight)
    {
        float usableHeight = clientHeight - 100;
        float floorHeight = usableHeight / FLOORS;
        float y = verticalOffset + 17 + (FLOORS - 1 - floor) * floorHeight;
        return y - elevatorHeight / 2;
    }

    void updateMovement() {
        int start = currentFloor;
        bool found = false;

        for (int f = 0; f < FLOORS; ++f) {
            if (onboardDest[f] > 0 && f != currentFloor) {
                found = true;
                if (f > currentFloor) movingUp = true;
                else if (f < currentFloor) movingUp = false;
                break;
            }
        }

        if (!found) {
            if (start == FLOORS - 1) movingUp = false;
            if (start == 0) movingUp = true;

            if (movingUp) {
                for (int f = start + 1; f < FLOORS; ++f)
                    if (waitingPeople[f] > 0) { found = true; break; }
                if (!found)
                    for (int f = start - 1; f >= 0; --f)
                        if (waitingPeople[f] > 0) { found = true; movingUp = false; break; }
            }
            else {
                for (int f = start - 1; f >= 0; --f)
                    if (waitingPeople[f] > 0) { found = true; break; }
                if (!found)
                    for (int f = start + 1; f < FLOORS; ++f)
                        if (waitingPeople[f] > 0) { found = true; movingUp = true; break; }
            }
        }
        moving = found;
    }

    void step(int clientHeight, int clientWidth) {
        float usableHeight = clientHeight - 100;
        float floorHeight = usableHeight / FLOORS;
        int elevW = clientWidth / 4;
        float doorX = clientWidth / 2 - elevW / 2 + elevW / 4;

        static DWORD stopTimer = 0;
        static bool atStop = false;

        float targetPos = floorToPos(currentFloor, clientHeight, floorHeight / 2);
        float speed = 4.0f;

        if (!atStop && std::fabs(position - targetPos) < speed &&
            (waitingPeople[currentFloor] > 0 || onboardDest[currentFloor] > 0))
        {
            position = targetPos;

            passengers -= onboardDest[currentFloor];
            onboardDest[currentFloor] = 0;

            boarding = true;
            stopTimer = GetTickCount();
            atStop = true;
            for (auto& p : people) p.justEnteredThisStop = false;
            return;
        }

        if (boarding) {
            for (auto& person : people) {
                if (person.state == InElevator && person.entered &&
                    currentFloor == person.toFloor) {
                    person.state = Exiting;
                    person.entered = false;
                }
            }
            people.erase(
                std::remove_if(people.begin(), people.end(), [&](const Person& p) {
                    return (p.state == Exiting && p.x == doorX && currentFloor == p.toFloor);
                }),
                people.end()
            );

            int peopleInElevator = 0;
            for (const auto& p : people)
                if (p.state == InElevator) peopleInElevator++;

            int peopleWalking = 0;
            for (const auto& p : people)
                if (p.state == Walking && p.fromFloor == currentFloor) peopleWalking++;

            for (auto& person : people) {
                if (person.state == Waiting &&
                    currentFloor == person.fromFloor &&
                    peopleInElevator + peopleWalking < capacity &&
                    !person.justEnteredThisStop) {
                    person.state = Walking;
                    person.justEnteredThisStop = true;
                    ++peopleWalking;
                }
            }

            for (auto& person : people) {
                if (person.state == Walking && person.fromFloor == currentFloor) {
                    if (fabs(person.x - doorX) > 2.0f) {
                        if (person.x < doorX) person.x += 2.0f;
                        else if (person.x > doorX) person.x -= 2.0f;
                    } else {
                        person.x = doorX;
                        person.state = InElevator;
                        person.entered = true;
                        onboardDest[person.toFloor]++;
                        passengers++;
                        waitingPeople[person.fromFloor] = std::max(0, waitingPeople[person.fromFloor]-1);
                        waitingPeopleDest[person.fromFloor][person.toFloor] = std::max(0, waitingPeopleDest[person.fromFloor][person.toFloor] - 1);
                    }
                }
            }

            bool anyoneWaiting = false, anyoneWalking = false;
            int currentInElevator = 0;
            for (const auto& p : people) {
                if (p.state == Waiting && p.fromFloor == currentFloor) anyoneWaiting = true;
                if (p.state == Walking && p.fromFloor == currentFloor) anyoneWalking = true;
                if (p.state == InElevator) currentInElevator++;
            }
            bool full = currentInElevator >= capacity;

            if ((!anyoneWaiting && !anyoneWalking) || full) {
                if (GetTickCount() - stopTimer > 500) {
                    boarding = false;
                    atStop = false;
                    updateMovement();
                }
            }
            return;
        }

        people.erase(
            std::remove_if(people.begin(), people.end(), [&](const Person& p) {
                return (p.state == Exiting && p.x == doorX && currentFloor == p.toFloor);
            }),
            people.end()
        );

        if (!moving) return;

        if (atStop) return;

        if (movingUp)
            position -= speed;
        else
            position += speed;

        float approxFloor = (FLOORS - 1) - ((position - verticalOffset - 17) / floorHeight);
        approxFloor = std::clamp(approxFloor, 0.0f, float(FLOORS - 1));
        currentFloor = int(approxFloor + 0.5f);
    }
};

Elevator elevator;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int clientWidth = 0;
    static int clientHeight = 0;

    switch (msg)
    {
    case WM_SIZE:
        clientWidth = LOWORD(lParam);
        clientHeight = HIWORD(lParam);
        elevator.setPositionByFloor(elevator.currentFloor, clientHeight);
        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_CREATE:
        SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);
        break;

    case WM_TIMER:
        if (wParam == TIMER_ID)
        {
            elevator.step(clientHeight, clientWidth);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int btnSize = 30;
        int btnSpacing = 10;

        float usableHeight = clientHeight - 100;
        float floorHeight = usableHeight / FLOORS;

        for (int f = 0; f < FLOORS; ++f)
        {
            int yBtnCenter = int(verticalOffset + 50 + f * floorHeight);
            int realFloor = FLOORS - 1 - f;

            int btnIndex = 0;
            for (int dest = 0; dest < FLOORS; ++dest)
            {
                if (dest == realFloor) continue;

                int btnX = 10 + btnIndex * (btnSize + btnSpacing);
                RECT btnRect = { btnX, yBtnCenter - btnSize / 2, btnX + btnSize, yBtnCenter + btnSize / 2 };

                if (x >= btnRect.left && x <= btnRect.right &&
                    y >= btnRect.top && y <= btnRect.bottom)
                {
                    elevator.call(realFloor, dest, clientWidth, clientHeight);
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;
                }

                ++btnIndex;
            }
        }
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        int width = rcClient.right - rcClient.left;
        int height = rcClient.bottom - rcClient.top;

        Bitmap buffer(width, height, PixelFormat32bppARGB);
        Graphics graphics(&buffer);
        SolidBrush bgBrush(Color(255, 240, 240, 240));
        graphics.FillRectangle(&bgBrush, 0, 0, width, height);

        Pen pen(Color(255, 0, 0, 0));
        FontFamily fontFamily(L"Arial");
        Font font(&fontFamily, 12, FontStyleRegular, UnitPixel);
        SolidBrush textBrush(Color(255, 0, 0, 0));

        float usableHeight = height - 100;
        float floorHeight = usableHeight / FLOORS;
        int btnSize = 30;
        int btnSpacing = 10;

        for (int f = 0; f < FLOORS; ++f)
        {
            int y = int(verticalOffset + 50 + f * floorHeight);
            int realFloor = FLOORS - 1 - f;

            graphics.DrawLine(&pen, 0, y, width, y);

            WCHAR buf[10];
            wsprintf(buf, L"%d", realFloor);
            graphics.DrawString(buf, -1, &font, PointF(5.0f, y + 10), &textBrush);

            SolidBrush btnActive(Color(255, 0, 128, 255));
            SolidBrush btnInactive(Color(255, 200, 200, 200));

            int btnXStart = 10;
            int btnIndex = 0;
            for (int dest = 0; dest < FLOORS; ++dest)
            {
                if (dest == realFloor) continue;

                int btnX = btnXStart + btnIndex * (btnSize + btnSpacing);
                Rect btnRect(btnX, y - btnSize / 2, btnSize, btnSize);

                bool active = elevator.waitingPeopleDest[realFloor][dest] > 0;
                if (active)
                    graphics.FillEllipse(&btnActive, btnRect);
                else
                    graphics.FillEllipse(&btnInactive, btnRect);
                graphics.DrawEllipse(&pen, btnRect);

                WCHAR destBuf[10];
                wsprintf(destBuf, L"%d", dest);
                graphics.DrawString(destBuf, -1, &font, PointF(btnX + 8, y - 8), &textBrush);

                ++btnIndex;
            }

            WCHAR countBuf[10];
            int sum = 0;
            for (int i = 0; i < FLOORS; ++i)
                sum += elevator.waitingPeopleDest[realFloor][i];
            wsprintf(countBuf, L"%d", sum);
            graphics.DrawString(countBuf, -1, &font, PointF(50, y - btnSize), &textBrush);
        }

        int elevW = width / 4;
        int elevH = floorHeight / 2;
        Rect elevRect(width / 2 - elevW / 2,
                      int(elevator.position),
                      elevW, int(elevH));
        graphics.DrawRectangle(&pen, elevRect);

        for (const auto& person : elevator.people) {
            if ((person.state == Waiting || person.state == Walking) && personImage) {
                graphics.DrawImage(personImage, (Gdiplus::REAL)person.x, (Gdiplus::REAL)person.y, (Gdiplus::REAL)32, (Gdiplus::REAL)32);
            }
        }
        int idx = 0;
        int maxCols = 5;
        int maxRows = (capacity + maxCols - 1) / maxCols;
        float iconW = elevW / (float)maxCols;
        float iconH = elevH / (float)maxRows;
        float iconSize = std::min(iconW, iconH);
        for (const auto& person : elevator.people) {
            if (person.state == InElevator && personImage) {
                int row = idx / maxCols;
                int col = idx % maxCols;
                float px = elevRect.X + col * iconW;
                float py = elevRect.Y + row * iconH;
                graphics.DrawImage(personImage, (Gdiplus::REAL)px, (Gdiplus::REAL)py, (Gdiplus::REAL)iconSize, (Gdiplus::REAL)iconSize);
                idx++;
            }
        }

        WCHAR passBuf[10];
        wsprintf(passBuf, L"%d", elevator.passengers);
        graphics.DrawString(passBuf, -1, &font,
            PointF(elevRect.X + elevW/2 - 6, elevRect.Y + elevH/2 - 6), &textBrush);

        WCHAR weightBuf[32];
        int totalWeight = elevator.passengers * PASSENGER_WEIGHT;
        wsprintf(weightBuf, L"Waga: %d kg", totalWeight);

        Font fontBig(&fontFamily, 16, FontStyleBold, UnitPixel);
        SolidBrush blackBrush(Color(255, 0, 0, 0));
        RectF layoutRect(width - 180.0f, 10.0f, 170.0f, 40.0f);
        graphics.DrawString(weightBuf, -1, &fontBig, layoutRect, NULL, &blackBrush);

        Graphics screenGraphics(hdc);
        screenGraphics.DrawImage(&buffer, 0, 0, width, height);
        EndPaint(hwnd, &ps);
    }
    break;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        GdiplusShutdown(gdiplusToken);
        if (personImage) {
            delete personImage;
            personImage = nullptr;
        }
        PostQuitMessage(0);
        break;

    case WM_ERASEBKGND:
        return 1;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    GdiplusStartupInput gdiplusStartupInput;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) {
        MessageBox(nullptr, L"Błąd inicjalizacji GDI+", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    personImage = new Image(L"person.png");
    if (personImage->GetLastStatus() != Ok) {
        MessageBox(nullptr, L"Nie można załadować person.png!", L"Błąd", MB_OK | MB_ICONERROR);
        return 1;
    }

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MyGDIApp";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc)) {
        MessageBox(nullptr, L"Rejestracja klasy okna nie powiodła się", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowEx(
        0, wc.lpszClassName, L"Symulator windy",
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 800,
        nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) {
        MessageBox(nullptr, L"Utworzenie okna nie powiodło się", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (personImage) {
        delete personImage;
        personImage = nullptr;
    }

    return static_cast<int>(msg.wParam);
}