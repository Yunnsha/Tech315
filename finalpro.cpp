
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <map>
#include <tuple>

using namespace std;
int romanToInt(const string &s) {
    map<char, int> val = {
        {'I', 1}, {'V', 5}, {'X', 10}, {'L', 50},
        {'C', 100}, {'D', 500}, {'M', 1000}
    };
    int total = 0;
    for (size_t i = 0; i < s.size(); i++) {
        int cur = val[s[i]];
        int nxt = (i + 1 < s.size()) ? val[s[i + 1]] : 0;
        if (cur < nxt) total -= cur;   // subtractive case, e.g. IV, CM
        else            total += cur;
    }
    return total;
}
string intToRoman(int num) {
    if (num <= 0 || num > 3999) return "";
    vector<pair<int, string>> table = {
        {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"},
        {100, "C"},  {90, "XC"},  {50, "L"},  {40, "XL"},
        {10, "X"},   {9, "IX"},   {5, "V"},   {4, "IV"},
        {1, "I"}
    };
    string result;
    for (auto &[value, symbol] : table) {
        while (num >= value) {
            result += symbol;
            num -= value;
        }
    }
    return result;
}


class RomanDFA {
public:
    enum State {
        S0, S1, S2, S3,                          // thousands: 0-3 M's
        HC1, HC2, HC3, HD, HDC1, HDC2, HDC3,      // hundreds counts
        HCD, HCM,                                 // hundreds special (400,900)
        TX1, TX2, TX3, TL, TLX1, TLX2, TLX3,      // tens counts
        TXC, TXL,                                 // tens special (90,40)
        UI1, UI2, UI3, UV, UVI1, UVI2, UVI3,      // units counts
        UIX, UIV,                                 // units special (9,4)
        NUM_STATES
    };
    static const int DEAD = -1;

    RomanDFA() { buildTable(); }

    // Returns true if s is accepted by the automaton.
    bool isValid(const string &s) const {
        if (s.empty()) return false;
        int state = S0;
        for (char c : s) {
            if (c < 0 || c >= 128) return false;
            int next = trans[state][(unsigned char)c];
            if (next == DEAD) return false;
            state = next;
        }
        return state != S0; // S0 itself is never an accepting state
    }

private:
    int trans[NUM_STATES][128];

    void buildTable() {
        memset(trans, DEAD, sizeof(trans));

        // ---- Thousands: S0 -M-> S1 -M-> S2 -M-> S3 (max 3 M's) ----
        trans[S0]['M'] = S1;
        trans[S1]['M'] = S2;
        trans[S2]['M'] = S3;
        // S3 has no 'M' transition -> a 4th M is rejected

        int thousandsStates[] = {S0, S1, S2, S3};
        for (int s : thousandsStates) {
            trans[s]['C'] = HC1;   // start of hundreds group
            trans[s]['D'] = HD;
            trans[s]['X'] = TX1;   // hundreds skipped entirely
            trans[s]['L'] = TL;
            trans[s]['I'] = UI1;   // hundreds & tens skipped entirely
            trans[s]['V'] = UV;
        }

        // ---- Hundreds counting / special forms ----
        trans[HC1]['C'] = HC2;
        trans[HC1]['D'] = HCD;     // "CD" = 400
        trans[HC1]['M'] = HCM;     // "CM" = 900
        trans[HC2]['C'] = HC3;
        // HC3: no further C/D/M (CCC is the max)

        trans[HD]['C']   = HDC1;
        trans[HDC1]['C'] = HDC2;
        trans[HDC2]['C'] = HDC3;
        // HDC3, HCD, HCM: hundreds section is closed, no more C/D/M

        int hundredsLevel[] = {HC1, HC2, HC3, HD, HDC1, HDC2, HDC3, HCD, HCM};
        for (int s : hundredsLevel) {
            trans[s]['X'] = TX1;   // move into tens
            trans[s]['L'] = TL;
            trans[s]['I'] = UI1;   // tens skipped entirely
            trans[s]['V'] = UV;
        }

        // ---- Tens counting / special forms ----
        trans[TX1]['C'] = TXC;     // "XC" = 90
        trans[TX1]['L'] = TXL;     // "XL" = 40
        trans[TX1]['X'] = TX2;
        trans[TX2]['X'] = TX3;
        // TX3: no further X (XXX is the max)

        trans[TL]['X']   = TLX1;
        trans[TLX1]['X'] = TLX2;
        trans[TLX2]['X'] = TLX3;
        // TLX3, TXC, TXL: tens section closed

        int tensLevel[] = {TX1, TX2, TX3, TL, TLX1, TLX2, TLX3, TXC, TXL};
        for (int s : tensLevel) {
            trans[s]['I'] = UI1;   // move into units
            trans[s]['V'] = UV;
        }

        // ---- Units counting / special forms ----
        trans[UI1]['X'] = UIX;     // "IX" = 9
        trans[UI1]['V'] = UIV;     // "IV" = 4
        trans[UI1]['I'] = UI2;
        trans[UI2]['I'] = UI3;
        // UI3: no further I (III is the max)

        trans[UV]['I']   = UVI1;
        trans[UVI1]['I'] = UVI2;
        trans[UVI2]['I'] = UVI3;
        // UVI3, UIX, UIV: units section closed - string must end here
    }
};

class RomanCFG {
public:
    bool isValid(const string &s) {
        if (s.empty()) return false;
        for (char c : s)
            if (string("MDCLXVI").find(c) == string::npos) return false;

        size_t pos = 0;
        parseThousands(s, pos);
        parseGroup(s, pos, 'C', 'D', 'M');   // hundreds: one='C' five='D' ten='M'
        parseGroup(s, pos, 'X', 'L', 'C');   // tens:     one='X' five='L' ten='C'
        parseGroup(s, pos, 'I', 'V', 'X');   // units:    one='I' five='V' ten='X'
        return pos == s.size();
    }

private:
    // Thousands -> 'M'{0,3}
    void parseThousands(const string &s, size_t &pos) {
        int count = 0;
        while (pos < s.size() && s[pos] == 'M' && count < 3) {
            pos++; count++;
        }
    }

    // Generic group parser for Hundreds / Tens / Units:
    //   Group -> one+ten | one+five | five? one{0,3}
    void parseGroup(const string &s, size_t &pos, char one, char five, char ten) {
        if (pos + 1 < s.size() && s[pos] == one && s[pos + 1] == ten) {
            pos += 2;               // e.g. "CM", "XC", "IX"
            return;
        }
        if (pos + 1 < s.size() && s[pos] == one && s[pos + 1] == five) {
            pos += 2;               // e.g. "CD", "XL", "IV"
            return;
        }
        if (pos < s.size() && s[pos] == five) {
            pos++;                  // optional five, e.g. "D", "L", "V"
        }
        int count = 0;
        while (pos < s.size() && s[pos] == one && count < 3) {
            pos++; count++;         // up to 3 of the base symbol
        }
    }
};


class RomanTM {
public:
    enum State {
        Q_S0, Q_S1, Q_S2, Q_S3,
        Q_HC1, Q_HC2, Q_HC3, Q_HD, Q_HDC1, Q_HDC2, Q_HDC3, Q_HCD, Q_HCM,
        Q_TX1, Q_TX2, Q_TX3, Q_TL, Q_TLX1, Q_TLX2, Q_TLX3, Q_TXC, Q_TXL,
        Q_UI1, Q_UI2, Q_UI3, Q_UV, Q_UVI1, Q_UVI2, Q_UVI3, Q_UIX, Q_UIV,
        Q_REJECT
    };

    RomanTM() { buildDelta(); }

    // Runs the machine on `input`; returns true (ACCEPT) or false (REJECT).
    // If verbose is true, prints each (state, tape symbol) transition.
    bool run(const string &input, bool verbose = false) {
        vector<char> tape(input.begin(), input.end());
        tape.push_back('_');           // blank marks end of tape
        size_t head = 0;
        State state = Q_S0;

        while (true) {
            char symbol = tape[head];
            if (symbol == '_') {
                bool accept = (state != Q_S0) && (state != Q_REJECT);
                if (verbose)
                    cout << "  [halt] blank read -> "
                         << (accept ? "ACCEPT" : "REJECT") << "\n";
                return accept;
            }
            auto key = make_pair(state, symbol);
            auto it = delta.find(key);
            if (it == delta.end()) {
                if (verbose) cout << "  [reject] no transition for symbol '"
                                   << symbol << "'\n";
                return false;
            }
            State newState = it->second.first;
            char writeSymbol = it->second.second;
            if (verbose)
                cout << "  state=" << state << " read='" << symbol
                     << "' -> write='" << writeSymbol
                     << "' newState=" << newState << " moveRight\n";
            tape[head] = writeSymbol;   // write (identical symbol here)
            state = newState;
            head++;                     // move right - always
        }
    }

private:
    map<pair<State, char>, pair<State, char>> delta;

    void add(State from, char c, State to) {
        delta[{from, c}] = {to, c}; // write back same symbol read
    }

    void buildDelta() {
        add(Q_S0, 'M', Q_S1); add(Q_S1, 'M', Q_S2); add(Q_S2, 'M', Q_S3);

        State thousands[] = {Q_S0, Q_S1, Q_S2, Q_S3};
        for (State s : thousands) {
            add(s, 'C', Q_HC1); add(s, 'D', Q_HD);
            add(s, 'X', Q_TX1); add(s, 'L', Q_TL);
            add(s, 'I', Q_UI1); add(s, 'V', Q_UV);
        }

        add(Q_HC1, 'C', Q_HC2); add(Q_HC1, 'D', Q_HCD); add(Q_HC1, 'M', Q_HCM);
        add(Q_HC2, 'C', Q_HC3);
        add(Q_HD, 'C', Q_HDC1);
        add(Q_HDC1, 'C', Q_HDC2);
        add(Q_HDC2, 'C', Q_HDC3);

        State hundredsLevel[] = {Q_HC1, Q_HC2, Q_HC3, Q_HD, Q_HDC1, Q_HDC2, Q_HDC3, Q_HCD, Q_HCM};
        for (State s : hundredsLevel) {
            add(s, 'X', Q_TX1); add(s, 'L', Q_TL);
            add(s, 'I', Q_UI1); add(s, 'V', Q_UV);
        }

        add(Q_TX1, 'C', Q_TXC); add(Q_TX1, 'L', Q_TXL); add(Q_TX1, 'X', Q_TX2);
        add(Q_TX2, 'X', Q_TX3);
        add(Q_TL, 'X', Q_TLX1);
        add(Q_TLX1, 'X', Q_TLX2);
        add(Q_TLX2, 'X', Q_TLX3);

        State tensLevel[] = {Q_TX1, Q_TX2, Q_TX3, Q_TL, Q_TLX1, Q_TLX2, Q_TLX3, Q_TXC, Q_TXL};
        for (State s : tensLevel) {
            add(s, 'I', Q_UI1); add(s, 'V', Q_UV);
        }

        add(Q_UI1, 'X', Q_UIX); add(Q_UI1, 'V', Q_UIV); add(Q_UI1, 'I', Q_UI2);
        add(Q_UI2, 'I', Q_UI3);
        add(Q_UV, 'I', Q_UVI1);
        add(Q_UVI1, 'I', Q_UVI2);
        add(Q_UVI2, 'I', Q_UVI3);
    }
};
void printSeparator() { cout << "------------------------------------------\n"; }

int main() {
    RomanDFA dfa;
    RomanCFG cfg;
    RomanTM  tm;

    int choice;
    do {
        printSeparator();
        cout << "ROMAN NUMERAL TOOLKIT\n";
        cout << "1) Validate a Roman numeral (DFA, CFG, and TM)\n";
        cout << "2) Convert Roman numeral -> Integer\n";
        cout << "3) Convert Integer -> Roman numeral\n";
        cout << "4) Show Turing Machine trace for a Roman numeral\n";
        cout << "0) Exit\n";
        cout << "Choose an option: ";
        if (!(cin >> choice)) return 0;

        if (choice == 1) {
            string s;
            cout << "Enter a Roman numeral: ";
            cin >> s;
            for (char &c : s) c = toupper(c);

            bool dfaResult = dfa.isValid(s);
            bool cfgResult = cfg.isValid(s);
            bool tmResult  = tm.run(s);

            cout << "\nInput: \"" << s << "\"\n";
            cout << "  DFA verdict : " << (dfaResult ? "VALID" : "INVALID") << "\n";
            cout << "  CFG verdict : " << (cfgResult ? "VALID" : "INVALID") << "\n";
            cout << "  TM  verdict : " << (tmResult  ? "VALID" : "INVALID") << "\n";

            if (dfaResult && cfgResult && tmResult) {
                cout << "  Integer value: " << romanToInt(s) << "\n";
            }
        } else if (choice == 2) {
            string s;
            cout << "Enter a Roman numeral: ";
            cin >> s;
            for (char &c : s) c = toupper(c);
            if (!dfa.isValid(s)) {
                cout << "\"" << s << "\" is not a valid Roman numeral.\n";
            } else {
                cout << s << " = " << romanToInt(s) << "\n";
            }
        } else if (choice == 3) {
            int n;
            cout << "Enter an integer (1-3999): ";
            cin >> n;
            string r = intToRoman(n);
            if (r.empty()) cout << "Out of range. Must be between 1 and 3999.\n";
            else cout << n << " = " << r << "\n";
        } else if (choice == 4) {
            string s;
            cout << "Enter a Roman numeral to trace through the TM: ";
            cin >> s;
            for (char &c : s) c = toupper(c);
            cout << "\nTuring Machine trace:\n";
            bool result = tm.run(s, true);
            cout << "Final verdict: " << (result ? "ACCEPT (valid)" : "REJECT (invalid)") << "\n";
        }
    } while (choice != 0);

    cout << "Goodbye!\n";
    return 0;
}