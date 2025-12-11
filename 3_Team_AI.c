/**
 * ================================================================================================
 * [C Command Battle AI Challenge] - 3_Team (Code Merge) - Enhanced Aggressive Version
 * ------------------------------------------------------------------------------------------------
 * 강화된 공격형 AI + 대각선 이동 + 상대 예측 + 맞대응 시스템
 * ================================================================================================
 */

#define _CRT_SECURE_NO_WARNINGS

#include "api.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* =================================================================================================
 * [공용 전역] - 내 등록 키 & 퍼즐 데이터 구조
 * ================================================================================================= */
static int MY_KEY = 0;

typedef struct {
    int  id;
    char name[50];
    char slot[10];
    int  atk;
    int  def;
    int  hp;
    char curse[50];
    char key_frag[50];
} PuzzleItem;

#define MAX_ITEMS      100
#define MAX_LINE_LEN   256

static PuzzleItem p_items[MAX_ITEMS];
static int  p_count = 0;
static char current_filename[100] = { 0 };

/* =================================================================================================
 * [상대방 움직임 예측 시스템] - 이전 턴 정보 저장
 * ================================================================================================= */
typedef struct {
    int prev_x;
    int prev_y;
    int prev_hp;
    int prev_mp;
    int move_dx;        // 상대 X 이동 방향 (-1, 0, 1)
    int move_dy;        // 상대 Y 이동 방향 (-1, 0, 1)
    int attack_pattern; // 0: 미확인, 1: 공격형, 2: 방어형, 3: 도주형
    int consecutive_approach; // 연속 접근 횟수
    int consecutive_retreat;  // 연속 후퇴 횟수
    int turn_count;     // 전체 턴 수
} OpponentHistory;

static OpponentHistory opp_history = { -1, -1, -1, -1, 0, 0, 0, 0, 0, 0 };

/* =================================================================================================
 * [PART 1] 퍼즐 솔버 & CSV 로더
 * ================================================================================================= */

static void load_csv_data(void) {
    const char* candidates[] = {
        "game_puzzle_en.csv",
        "AI1-2_C_Final.csv",
        "puzzle.csv",
        "data.csv"
    };

    FILE* fp = NULL;
    int num_candidates = (int)(sizeof(candidates) / sizeof(candidates[0]));

    for (int i = 0; i < num_candidates; i++) {
        fp = fopen(candidates[i], "r");
        if (fp != NULL) {
            strcpy(current_filename, candidates[i]);
            printf("SYSTEM: CSV Loaded [%s]\n", current_filename);
            break;
        }
    }

    if (fp == NULL) {
        printf("ERROR: CSV file not found!\n");
        return;
    }

    char line[MAX_LINE_LEN];
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return; }

    p_count = 0;
    while (fgets(line, sizeof(line), fp) && p_count < MAX_ITEMS) {
        PuzzleItem* it = &p_items[p_count];
        char temp[MAX_LINE_LEN];
        strcpy(temp, line);

        char* tok = strtok(temp, ","); if (!tok) continue;
        it->id = atoi(tok);

        tok = strtok(NULL, ","); if (!tok) continue;
        strcpy(it->name, tok);

        tok = strtok(NULL, ","); if (!tok) continue;
        strcpy(it->slot, tok);

        tok = strtok(NULL, ","); if (!tok) continue;
        it->atk = atoi(tok);

        tok = strtok(NULL, ","); if (!tok) continue;
        it->def = atoi(tok);

        tok = strtok(NULL, ","); if (!tok) continue;
        it->hp = atoi(tok);

        tok = strtok(NULL, ","); if (!tok) continue;
        strcpy(it->curse, tok);

        tok = strtok(NULL, ","); if (!tok) continue;
        tok[strcspn(tok, "\r\n")] = 0;
        strcpy(it->key_frag, tok);

        p_count++;
    }
    fclose(fp);
}

static PuzzleItem* get_item(int id) {
    for (int i = 0; i < p_count; i++) {
        if (p_items[i].id == id) return &p_items[i];
    }
    return NULL;
}

/* 퀴즈 솔버 함수들 */
static void solve_poison(char* buf) {
    buf[0] = 0;
    int idx[MAX_ITEMS];
    int c = 0;
    for (int i = 0; i < p_count; i++) {
        if (p_items[i].atk >= 4 && p_items[i].def <= 5 && p_items[i].hp <= 100) {
            idx[c++] = i;
        }
    }
    for (int i = c - 1; i >= 0; i--) {
        strcat(buf, p_items[idx[i]].name);
        if (i > 0) strcat(buf, "|");
    }
}

static void solve_strike(char* buf) {
    int sum = 0;
    for (int i = 0; i < p_count; i++) {
        if (strcmp(p_items[i].slot, "W") == 0) {
            char* p = strchr(p_items[i].key_frag, 'T');
            if (p) sum += (int)(p - p_items[i].key_frag);
        }
    }
    sprintf(buf, "%dkey", sum);
}

static void solve_blink(char* buf) {
    buf[0] = 0;
    PuzzleItem* i202 = get_item(202);
    PuzzleItem* i208 = get_item(208);
    PuzzleItem* i205 = get_item(205);
    PuzzleItem* i212 = get_item(212);
    if (!i202 || !i208 || !i205 || !i212) return;

    int thp = i202->def + i208->def;
    char* key1 = "";
    for (int i = 0; i < p_count; i++) {
        if (p_items[i].hp == thp && strcmp(p_items[i].key_frag, "NIL") != 0) {
            key1 = p_items[i].key_frag;
        }
    }
    strcat(buf, key1);

    int tatk = i205->atk * i212->atk;
    char* key2 = "";
    for (int i = 0; i < p_count; i++) {
        if (p_items[i].atk == tatk && strcmp(p_items[i].key_frag, "NIL") != 0) {
            key2 = p_items[i].key_frag;
        }
    }
    strcat(buf, key2);

    int fidx = -1;
    for (int i = 0; i < p_count; i++) {
        if (strstr(p_items[i].curse, "C_01") && strcmp(p_items[i].key_frag, "NIL") != 0) {
            fidx = i;
        }
    }
    if (fidx != -1) strcat(buf, p_items[fidx].key_frag);

    for (int i = 0; i < p_count; i++) {
        if (p_items[i].name[0] == 'I' && strcmp(p_items[i].key_frag, "NIL") != 0) {
            strcat(buf, p_items[i].key_frag);
            break;
        }
    }
}

static void solve_heal_all(char* buf) {
    buf[0] = 0;
    for (int i = 0; i < p_count; i++) {
        if (strcmp(p_items[i].name, p_items[i].slot) >= 0) {
            strcpy(buf, p_items[i].key_frag);
            return;
        }
    }
}

static void solve_range(char* buf) {
    buf[0] = 0;
    int nv = 0;

    for (int i = 0; i < p_count; i++) {
        if (strstr(p_items[i].key_frag, "K") != NULL) {
            nv = p_items[i].hp;
            break;
        }
    }
    if (nv <= 0 || current_filename[0] == '\0') return;

    FILE* fp = fopen(current_filename, "rb");
    if (!fp) return;

    if (fseek(fp, nv - 1, SEEK_SET) == 0) {
        char t[6] = { 0 };
        size_t r = fread(t, 1, 5, fp);
        t[r] = '\0';
        sprintf(buf, "\"%s\"", t);
    }
    fclose(fp);
}

static void solve_self_destruct(char* buf) {
    buf[0] = 0;
    char all[2000] = { 0 };
    for (int i = 0; i < p_count; i++) {
        if (strstr(p_items[i].name, "Sword") && strcmp(p_items[i].key_frag, "NIL") != 0) {
            strcat(all, p_items[i].key_frag);
        }
    }
    char* tok = strtok(all, "*");
    char* best = NULL;
    int best_len = -1;
    while (tok) {
        int len = (int)strlen(tok);
        if (len > best_len) {
            best_len = len;
            best = tok;
        }
        tok = strtok(NULL, "*");
    }
    if (best) strcpy(buf, best);
}

static void solve_hv(char* buf) {
    buf[0] = 0;
    int max_name_len = -1, min_curse_len = 999;
    int idx_name = -1, idx_curse = -1;
    for (int i = 0; i < p_count; i++) {
        int nl = (int)strlen(p_items[i].name);
        int cl = (int)strlen(p_items[i].curse);
        if (nl > max_name_len) { max_name_len = nl; idx_name = i; }
        if (cl < min_curse_len) { min_curse_len = cl; idx_curse = i; }
    }
    if (idx_name == -1 || idx_curse == -1) return;

    char p1[4] = { 0 }, p2[4] = { 0 };
    strncpy(p1, p_items[idx_name].name, 3);
    int cl = (int)strlen(p_items[idx_curse].curse);
    if (cl >= 3) {
        strncpy(p2, p_items[idx_curse].curse + (cl - 3), 3);
    }
    else {
        strcpy(p2, p_items[idx_curse].curse);
    }
    sprintf(buf, "%s%s", p1, p2);
}

static void solve_secret(char* buf) {
    buf[0] = 0;
    char tn[50] = { 0 };
    for (int i = 0; i < p_count; i++) {
        if (strstr(p_items[i].name, "Stone")) {
            strcpy(tn, p_items[i].name);
            break;
        }
    }
    if (tn[0] == '\0') return;

    char temp_tn[50];
    strcpy(temp_tn, tn);

    char* tok = strtok(temp_tn, "AEIOUaeiou");
    char* best = NULL;
    int best_len = -1;
    while (tok) {
        int len = (int)strlen(tok);
        if (len > best_len) {
            best_len = len;
            best = tok;
        }
        tok = strtok(NULL, "AEIOUaeiou");
    }
    if (best) strcpy(buf, best);
}

/* =================================================================================================
 * [PART 2] 유틸리티 함수들
 * ================================================================================================= */

static int get_dist(const Player* p1, const Player* p2) {
    int dx = get_player_x(p1) - get_player_x(p2);
    int dy = get_player_y(p1) - get_player_y(p2);
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx + dy;
}

static int abs_val(int x) {
    return (x < 0) ? -x : x;
}

static int sign(int x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

/* =================================================================================================
 * [PART 3] 상대방 움직임 예측 시스템
 * ================================================================================================= */

static void update_opponent_history(int opp_x, int opp_y, int opp_hp, int opp_mp, int my_x, int my_y) {
    opp_history.turn_count++;

    if (opp_history.prev_x >= 0 && opp_history.prev_y >= 0) {
        // 이동 방향 계산
        opp_history.move_dx = sign(opp_x - opp_history.prev_x);
        opp_history.move_dy = sign(opp_y - opp_history.prev_y);

        // 접근/후퇴 패턴 분석
        int prev_dist_to_me = abs_val(opp_history.prev_x - my_x) + abs_val(opp_history.prev_y - my_y);
        int curr_dist_to_me = abs_val(opp_x - my_x) + abs_val(opp_y - my_y);

        if (curr_dist_to_me < prev_dist_to_me) {
            // 상대가 접근 중
            opp_history.consecutive_approach++;
            opp_history.consecutive_retreat = 0;
            if (opp_history.consecutive_approach >= 2) {
                opp_history.attack_pattern = 1; // 공격형
            }
        } else if (curr_dist_to_me > prev_dist_to_me) {
            // 상대가 후퇴 중
            opp_history.consecutive_retreat++;
            opp_history.consecutive_approach = 0;
            if (opp_history.consecutive_retreat >= 2) {
                opp_history.attack_pattern = 3; // 도주형
            }
        } else {
            // HP 회복이나 제자리
            if (opp_hp > opp_history.prev_hp) {
                opp_history.attack_pattern = 2; // 방어형 (회복 중)
            }
        }
    }

    // 현재 상태 저장
    opp_history.prev_x = opp_x;
    opp_history.prev_y = opp_y;
    opp_history.prev_hp = opp_hp;
    opp_history.prev_mp = opp_mp;
}

// 상대방 다음 위치 예측
static void predict_opponent_next_pos(int opp_x, int opp_y, int* pred_x, int* pred_y) {
    *pred_x = opp_x + opp_history.move_dx;
    *pred_y = opp_y + opp_history.move_dy;

    // 맵 경계 처리
    if (*pred_x < 1) *pred_x = 1;
    if (*pred_x > 7) *pred_x = 7;
    if (*pred_y < 1) *pred_y = 1;
    if (*pred_y > 7) *pred_y = 7;
}

/* =================================================================================================
 * [PART 4] 대각선 이동 및 포지셔닝 시스템
 * ================================================================================================= */

// 대각선 이동: 두 번의 이동 명령 반환 (첫 번째 이동만 사용)
static int get_diagonal_move_toward(int my_x, int my_y, int opp_x, int opp_y) {
    int dx = opp_x - my_x;
    int dy = opp_y - my_y;

    // 대각선 접근: X와 Y 모두 차이가 있으면 더 먼 축 우선
    if (dx != 0 && dy != 0) {
        if (abs_val(dx) >= abs_val(dy)) {
            // X축 먼저
            if (dx > 0 && my_x < 7) return CMD_RIGHT;
            if (dx < 0 && my_x > 1) return CMD_LEFT;
        }
        // Y축 먼저
        if (dy > 0 && my_y < 7) return CMD_DOWN;
        if (dy < 0 && my_y > 1) return CMD_UP;
    }

    // 일직선이면 그냥 접근
    if (dx > 0 && my_x < 7) return CMD_RIGHT;
    if (dx < 0 && my_x > 1) return CMD_LEFT;
    if (dy > 0 && my_y < 7) return CMD_DOWN;
    if (dy < 0 && my_y > 1) return CMD_UP;

    return CMD_REST;
}

// 대각선 공격 위치로 이동 (H_ATTACK 또는 V_ATTACK 사정거리에 들어가기)
static int move_to_attack_position(int my_x, int my_y, int opp_x, int opp_y, int has_h, int has_v) {
    // H_ATTACK 가능 위치: 같은 Y축
    // V_ATTACK 가능 위치: 같은 X축

    int dx = opp_x - my_x;
    int dy = opp_y - my_y;

    // 이미 같은 라인에 있으면 그대로
    if (has_h && my_y == opp_y && abs_val(dx) >= 2) return 0; // 공격 가능
    if (has_v && my_x == opp_x && abs_val(dy) >= 2) return 0; // 공격 가능

    // H_ATTACK 사정거리로 이동 (Y축 맞추기)
    if (has_h && my_y != opp_y) {
        if (opp_y > my_y && my_y < 7) return CMD_DOWN;
        if (opp_y < my_y && my_y > 1) return CMD_UP;
    }

    // V_ATTACK 사정거리로 이동 (X축 맞추기)
    if (has_v && my_x != opp_x) {
        if (opp_x > my_x && my_x < 7) return CMD_RIGHT;
        if (opp_x < my_x && my_x > 1) return CMD_LEFT;
    }

    return -1; // 이동 필요 없음
}

// 측면(대각선) 공격 포지션 확보
static int get_flanking_position(int my_x, int my_y, int opp_x, int opp_y) {
    // 상대 바로 옆이 아닌 대각선 위치로 이동
    // 목표: 상대의 대각선 위치에서 공격 기회 노리기

    int dx = opp_x - my_x;
    int dy = opp_y - my_y;

    // 이미 근접해 있으면 측면으로 이동
    if (abs_val(dx) <= 1 && abs_val(dy) <= 1) {
        // 대각선 위치 확보
        if (dx == 0 && dy != 0) {
            // 수직 정렬 -> 좌우로 이동
            if (my_x < 7) return CMD_RIGHT;
            if (my_x > 1) return CMD_LEFT;
        }
        if (dy == 0 && dx != 0) {
            // 수평 정렬 -> 상하로 이동
            if (my_y < 7) return CMD_DOWN;
            if (my_y > 1) return CMD_UP;
        }
    }

    return 0; // 이미 좋은 위치
}

/* =================================================================================================
 * [PART 5] BLINK 이동 함수
 * ================================================================================================= */

static int get_safe_blink_away(int my_x, int my_y, int opp_x, int opp_y,
                               int has_blink_u, int has_blink_d, int has_blink_l, int has_blink_r) {
    if (opp_x < my_x && my_x + 2 <= 7 && has_blink_r) return CMD_BLINK_RIGHT;
    if (opp_x > my_x && my_x - 2 >= 1 && has_blink_l) return CMD_BLINK_LEFT;
    if (opp_y < my_y && my_y + 2 <= 7 && has_blink_d) return CMD_BLINK_DOWN;
    if (opp_y > my_y && my_y - 2 >= 1 && has_blink_u) return CMD_BLINK_UP;
    if (my_x + 2 <= 7 && has_blink_r) return CMD_BLINK_RIGHT;
    if (my_x - 2 >= 1 && has_blink_l) return CMD_BLINK_LEFT;
    if (my_y + 2 <= 7 && has_blink_d) return CMD_BLINK_DOWN;
    if (my_y - 2 >= 1 && has_blink_u) return CMD_BLINK_UP;
    return 0;
}

static int get_safe_blink_toward(int my_x, int my_y, int opp_x, int opp_y,
                                  int has_blink_u, int has_blink_d, int has_blink_l, int has_blink_r) {
    if (opp_x > my_x && my_x + 2 <= 7 && has_blink_r) return CMD_BLINK_RIGHT;
    if (opp_x < my_x && my_x - 2 >= 1 && has_blink_l) return CMD_BLINK_LEFT;
    if (opp_y > my_y && my_y + 2 <= 7 && has_blink_d) return CMD_BLINK_DOWN;
    if (opp_y < my_y && my_y - 2 >= 1 && has_blink_u) return CMD_BLINK_UP;
    if (my_x + 2 <= 7 && has_blink_r) return CMD_BLINK_RIGHT;
    if (my_x - 2 >= 1 && has_blink_l) return CMD_BLINK_LEFT;
    if (my_y + 2 <= 7 && has_blink_d) return CMD_BLINK_DOWN;
    if (my_y - 2 >= 1 && has_blink_u) return CMD_BLINK_UP;
    return 0;
}

// 대각선 BLINK: H/V 공격 사정거리로 순간이동
static int get_blink_to_attack_line(int my_x, int my_y, int opp_x, int opp_y,
                                     int has_blink_u, int has_blink_d, int has_blink_l, int has_blink_r,
                                     int has_h, int has_v) {
    // H_ATTACK 사정거리(같은 Y축)로 BLINK
    if (has_h) {
        if (opp_y > my_y && my_y + 2 <= 7 && has_blink_d) {
            int new_y = my_y + 2;
            if (new_y == opp_y || abs_val(new_y - opp_y) <= 1) return CMD_BLINK_DOWN;
        }
        if (opp_y < my_y && my_y - 2 >= 1 && has_blink_u) {
            int new_y = my_y - 2;
            if (new_y == opp_y || abs_val(new_y - opp_y) <= 1) return CMD_BLINK_UP;
        }
    }

    // V_ATTACK 사정거리(같은 X축)로 BLINK
    if (has_v) {
        if (opp_x > my_x && my_x + 2 <= 7 && has_blink_r) {
            int new_x = my_x + 2;
            if (new_x == opp_x || abs_val(new_x - opp_x) <= 1) return CMD_BLINK_RIGHT;
        }
        if (opp_x < my_x && my_x - 2 >= 1 && has_blink_l) {
            int new_x = my_x - 2;
            if (new_x == opp_x || abs_val(new_x - opp_x) <= 1) return CMD_BLINK_LEFT;
        }
    }

    return 0;
}

/* =================================================================================================
 * [PART 6] 킬 판단 시스템 (대각선 vs 직선)
 * ================================================================================================= */

typedef struct {
    int can_kill;           // 죽일 수 있는가
    int method;             // 0: 불가, 1: 근접, 2: STRIKE, 3: RANGE, 4: H_ATTACK, 5: V_ATTACK
    int cmd;                // 실행할 명령
    int moves_needed;       // 필요한 이동 수
    int mp_cost;            // MP 비용
} KillAnalysis;

static KillAnalysis analyze_kill_options(int my_x, int my_y, int my_mp, int opp_x, int opp_y, int opp_hp,
                                          int has_strike, int has_range, int has_h, int has_v) {
    KillAnalysis result = { 0, 0, CMD_REST, 999, 0 };
    int dx = abs_val(opp_x - my_x);
    int dy = abs_val(opp_y - my_y);
    int dist = dx + dy;

    // 1. 근접 공격 (거리 1, 데미지 1)
    if (opp_hp <= 1 && dist == 1 && my_mp >= 0) {
        result.can_kill = 1;
        result.method = 1;
        result.cmd = CMD_ATTACK;
        result.moves_needed = 0;
        result.mp_cost = 0;
        return result;
    }

    // 2. STRIKE (거리 1, 데미지 2)
    if (has_strike && opp_hp <= 2 && dist == 1 && my_mp >= 2) {
        result.can_kill = 1;
        result.method = 2;
        result.cmd = CMD_STRIKE;
        result.moves_needed = 0;
        result.mp_cost = 2;
        return result;
    }

    // 3. RANGE_ATTACK (거리 2, 데미지 1)
    if (has_range && opp_hp <= 1 && dist == 2 && my_mp >= 1) {
        result.can_kill = 1;
        result.method = 3;
        result.cmd = CMD_RANGE_ATTACK;
        result.moves_needed = 0;
        result.mp_cost = 1;
        return result;
    }

    // 4. H_ATTACK (같은 Y축, 거리 상관없이, 데미지 2)
    if (has_h && opp_hp <= 2 && my_y == opp_y && dx >= 2 && my_mp >= 3) {
        result.can_kill = 1;
        result.method = 4;
        result.cmd = CMD_H_ATTACK;
        result.moves_needed = 0;
        result.mp_cost = 3;
        return result;
    }

    // 5. V_ATTACK (같은 X축, 거리 상관없이, 데미지 2)
    if (has_v && opp_hp <= 2 && my_x == opp_x && dy >= 2 && my_mp >= 3) {
        result.can_kill = 1;
        result.method = 5;
        result.cmd = CMD_V_ATTACK;
        result.moves_needed = 0;
        result.mp_cost = 3;
        return result;
    }

    // 6. 이동 후 킬 가능 분석
    // H_ATTACK 사정거리로 이동 후 킬
    if (has_h && opp_hp <= 2 && my_y != opp_y && my_mp >= 3) {
        int moves = abs_val(opp_y - my_y);
        if (moves < result.moves_needed) {
            result.can_kill = 1;
            result.method = 4;
            result.moves_needed = moves;
            result.mp_cost = 3;
            if (opp_y > my_y) result.cmd = CMD_DOWN;
            else result.cmd = CMD_UP;
        }
    }

    // V_ATTACK 사정거리로 이동 후 킬
    if (has_v && opp_hp <= 2 && my_x != opp_x && my_mp >= 3) {
        int moves = abs_val(opp_x - my_x);
        if (moves < result.moves_needed) {
            result.can_kill = 1;
            result.method = 5;
            result.moves_needed = moves;
            result.mp_cost = 3;
            if (opp_x > my_x) result.cmd = CMD_RIGHT;
            else result.cmd = CMD_LEFT;
        }
    }

    return result;
}

/* =================================================================================================
 * [PART 7] 맞대응(Counter-Attack) 시스템
 * ================================================================================================= */

static int get_counter_attack(int my_hp, int my_mp, int my_x, int my_y,
                               int opp_hp, int opp_x, int opp_y, int dist,
                               int has_strike, int has_range, int has_h, int has_v,
                               int has_blink_u, int has_blink_d, int has_blink_l, int has_blink_r) {

    // 상대 패턴에 따른 맞대응
    switch (opp_history.attack_pattern) {
        case 1: // 공격형 상대 -> 선제 타격 or 회피 후 반격
            if (opp_history.consecutive_approach >= 2) {
                // 상대가 계속 접근 중 -> 기습 공격
                if (dist == 1 && has_strike && my_mp >= 2) {
                    return CMD_STRIKE; // 강타로 선제
                }
                if (dist == 2 && has_range && my_mp >= 1) {
                    return CMD_RANGE_ATTACK; // 원거리 견제
                }
                // 라인 공격 기습
                if (my_y == opp_y && has_h && my_mp >= 3) return CMD_H_ATTACK;
                if (my_x == opp_x && has_v && my_mp >= 3) return CMD_V_ATTACK;
            }
            break;

        case 2: // 방어형 상대 -> 공세 강화
            // 상대가 회복 중이면 추격해서 압박
            if (my_mp >= 2) {
                int blink = get_safe_blink_toward(my_x, my_y, opp_x, opp_y,
                                                   has_blink_u, has_blink_d, has_blink_l, has_blink_r);
                if (blink > 0) return blink;
            }
            break;

        case 3: // 도주형 상대 -> 추격 + 원거리 공격
            if (opp_history.consecutive_retreat >= 2) {
                // 라인 공격으로 도망치는 상대 저격
                if (my_y == opp_y && has_h && my_mp >= 3) return CMD_H_ATTACK;
                if (my_x == opp_x && has_v && my_mp >= 3) return CMD_V_ATTACK;
                // BLINK로 추격
                if (my_mp >= 1) {
                    int blink = get_safe_blink_toward(my_x, my_y, opp_x, opp_y,
                                                       has_blink_u, has_blink_d, has_blink_l, has_blink_r);
                    if (blink > 0) return blink;
                }
            }
            break;
    }

    return 0; // 특별한 맞대응 없음
}

/* =================================================================================================
 * [PART 8] 3_Team 전투 AI - 강화된 공격형
 * ================================================================================================= */

static int my_ai(const Player* me, const Player* opp) {
    // 기본 정보 수집
    int my_hp = get_player_hp(me);
    int my_mp = get_player_mp(me);
    int my_x = get_player_x(me);
    int my_y = get_player_y(me);
    int opp_hp = get_player_hp(opp);
    int opp_mp = get_player_mp(opp);
    int opp_x = get_player_x(opp);
    int opp_y = get_player_y(opp);
    int dist = get_dist(me, opp);

    int hp_diff = my_hp - opp_hp;
    int mp_diff = my_mp - opp_mp;

    // 스킬 해금 여부 확인 (모든 스킬)
    int has_poison = is_skill_unlocked(MY_KEY, CMD_POISON);
    int has_strike = is_skill_unlocked(MY_KEY, CMD_STRIKE);
    int has_blink_u = is_skill_unlocked(MY_KEY, CMD_BLINK_UP);
    int has_blink_d = is_skill_unlocked(MY_KEY, CMD_BLINK_DOWN);
    int has_blink_l = is_skill_unlocked(MY_KEY, CMD_BLINK_LEFT);
    int has_blink_r = is_skill_unlocked(MY_KEY, CMD_BLINK_RIGHT);
    int has_heal_all = is_skill_unlocked(MY_KEY, CMD_HEAL_ALL);
    int has_range = is_skill_unlocked(MY_KEY, CMD_RANGE_ATTACK);
    int has_bless = is_skill_unlocked(MY_KEY, CMD_BLESS);
    int has_h_attack = is_skill_unlocked(MY_KEY, CMD_H_ATTACK);
    int has_v_attack = is_skill_unlocked(MY_KEY, CMD_V_ATTACK);
    int has_any_blink = has_blink_u || has_blink_d || has_blink_l || has_blink_r;

    // 상대방 움직임 기록 업데이트
    update_opponent_history(opp_x, opp_y, opp_hp, opp_mp, my_x, my_y);

    // 상대 다음 위치 예측
    int pred_opp_x, pred_opp_y;
    predict_opponent_next_pos(opp_x, opp_y, &pred_opp_x, &pred_opp_y);

    // 죽은 상태면 REST
    if (my_hp <= 0) return CMD_REST;

    /* =====================================================================================
     * PHASE 1: 즉시 킬각 판단 (대각선 vs 직선)
     * ===================================================================================== */
    KillAnalysis kill = analyze_kill_options(my_x, my_y, my_mp, opp_x, opp_y, opp_hp,
                                              has_strike, has_range, has_h_attack, has_v_attack);

    if (kill.can_kill && kill.moves_needed == 0) {
        // 즉시 킬 가능!
        return kill.cmd;
    }

    // 1턴 후 킬 가능: 먼저 이동
    if (kill.can_kill && kill.moves_needed == 1 && kill.cmd != CMD_REST) {
        return kill.cmd; // 이동 명령 반환
    }

    /* =====================================================================================
     * PHASE 2: 예측 기반 선제 공격
     * ===================================================================================== */
    // 상대가 예측 위치로 이동할 것이라고 가정하고 선제 공격
    if (opp_history.turn_count >= 2) {
        // 예측 위치에 H_ATTACK/V_ATTACK 가능하면 선제
        if (has_h_attack && my_mp >= 3 && my_y == pred_opp_y && abs_val(pred_opp_x - my_x) >= 2) {
            return CMD_H_ATTACK;
        }
        if (has_v_attack && my_mp >= 3 && my_x == pred_opp_x && abs_val(pred_opp_y - my_y) >= 2) {
            return CMD_V_ATTACK;
        }
    }

    /* =====================================================================================
     * PHASE 3: 맞대응 시스템 (상대 패턴 기반)
     * ===================================================================================== */
    int counter = get_counter_attack(my_hp, my_mp, my_x, my_y, opp_hp, opp_x, opp_y, dist,
                                      has_strike, has_range, has_h_attack, has_v_attack,
                                      has_blink_u, has_blink_d, has_blink_l, has_blink_r);
    if (counter > 0) {
        return counter;
    }

    /* =====================================================================================
     * PHASE 4: 위기 상황 대응
     * ===================================================================================== */
    int critical_hp = (my_hp <= 1);
    int danger_hp = (my_hp <= 2);
    int low_hp = (my_hp <= 3 && hp_diff <= -1);

    // 4-1. 치명적 위기 (HP 1): HEAL_ALL 우선, 없으면 HEAL, 없으면 도주
    if (critical_hp) {
        if (has_heal_all && my_mp >= 3) return CMD_HEAL_ALL;
        if (my_mp >= 1) return CMD_HEAL;
        // BLINK 도주
        int blink = get_safe_blink_away(my_x, my_y, opp_x, opp_y,
                                         has_blink_u, has_blink_d, has_blink_l, has_blink_r);
        if (blink > 0 && my_mp >= 1) return blink;
        // 이동 도주
        if (opp_x < my_x && my_x < 7) return CMD_RIGHT;
        if (opp_x > my_x && my_x > 1) return CMD_LEFT;
        if (opp_y < my_y && my_y < 7) return CMD_DOWN;
        if (opp_y > my_y && my_y > 1) return CMD_UP;
    }

    // 4-2. 위험 상황 (HP 2): 상황에 따라 공격 or 회복
    if (danger_hp) {
        // 상대도 HP 낮으면 공격 우선 (서로 죽음 상황)
        if (opp_hp <= 2 && dist <= 1) {
            if (has_strike && my_mp >= 2) return CMD_STRIKE;
            return CMD_ATTACK;
        }
        // 원거리에서 안전하게 회복
        if (dist >= 3 && my_mp >= 1) return CMD_HEAL;
        if (has_heal_all && my_mp >= 3) return CMD_HEAL_ALL;
        if (my_mp >= 1) return CMD_HEAL;
    }

    // 4-3. 불리한 상황 (HP 3 이하, 열세)
    if (low_hp) {
        // 원거리 공격으로 안전하게 데미지
        if (my_y == opp_y && has_h_attack && my_mp >= 3 && dist >= 2) return CMD_H_ATTACK;
        if (my_x == opp_x && has_v_attack && my_mp >= 3 && dist >= 2) return CMD_V_ATTACK;
        // 거리 벌리고 회복
        if (dist <= 2) {
            int blink = get_safe_blink_away(my_x, my_y, opp_x, opp_y,
                                             has_blink_u, has_blink_d, has_blink_l, has_blink_r);
            if (blink > 0 && my_mp >= 1) return blink;
        }
        if (my_mp >= 1 && my_hp < 5) return CMD_HEAL;
    }

    /* =====================================================================================
     * PHASE 5: 공격 우세 상황 - 최대 공격력
     * ===================================================================================== */
    int advantage = (hp_diff >= 2) || (my_hp >= 4 && opp_hp <= 2);

    if (advantage) {
        // 5-1. 라인 공격 (최대 데미지)
        if (my_y == opp_y && has_h_attack && my_mp >= 3) return CMD_H_ATTACK;
        if (my_x == opp_x && has_v_attack && my_mp >= 3) return CMD_V_ATTACK;

        // 5-2. 근접전 강화
        if (dist == 1) {
            if (has_strike && my_mp >= 2) return CMD_STRIKE;
            return CMD_ATTACK;
        }

        // 5-3. 거리 2: Range 공격
        if (dist == 2 && has_range && my_mp >= 1) return CMD_RANGE_ATTACK;

        // 5-4. BLINK로 빠르게 추격
        if (dist >= 3 && my_mp >= 1 && has_any_blink) {
            int blink = get_safe_blink_toward(my_x, my_y, opp_x, opp_y,
                                               has_blink_u, has_blink_d, has_blink_l, has_blink_r);
            if (blink > 0) return blink;
        }

        // 5-5. 라인 공격 사정거리로 BLINK
        if (my_mp >= 1 && has_any_blink && (has_h_attack || has_v_attack)) {
            int blink = get_blink_to_attack_line(my_x, my_y, opp_x, opp_y,
                                                  has_blink_u, has_blink_d, has_blink_l, has_blink_r,
                                                  has_h_attack, has_v_attack);
            if (blink > 0) return blink;
        }
    }

    /* =====================================================================================
     * PHASE 6: 일반 전투 - 대각선 포지셔닝 포함
     * ===================================================================================== */

    // 6-1. 라인 공격 기회
    if (my_y == opp_y && has_h_attack && my_mp >= 3 && dist >= 2) return CMD_H_ATTACK;
    if (my_x == opp_x && has_v_attack && my_mp >= 3 && dist >= 2) return CMD_V_ATTACK;

    // 6-2. 근접전 (dist == 1)
    if (dist == 1) {
        // POISON 사용 (상대 HP 높을 때)
        if (has_poison && my_mp >= 5 && opp_hp >= 3) return CMD_POISON;
        // STRIKE 강타
        if (has_strike && my_mp >= 2 && opp_hp >= 2) return CMD_STRIKE;
        // 일반 공격
        return CMD_ATTACK;
    }

    // 6-3. 거리 2: Range 공격 or 접근
    if (dist == 2) {
        if (has_range && my_mp >= 1) return CMD_RANGE_ATTACK;
        // 대각선 접근
        return get_diagonal_move_toward(my_x, my_y, opp_x, opp_y);
    }

    // 6-4. 원거리: 라인 공격 사정거리로 이동 (대각선 포지셔닝)
    if (dist >= 3) {
        // 라인 공격 사정거리로 이동 시도
        int line_move = move_to_attack_position(my_x, my_y, opp_x, opp_y, has_h_attack, has_v_attack);
        if (line_move > 0) return line_move;

        // BLINK로 빠르게 접근
        if (my_mp >= 1 && has_any_blink) {
            int blink = get_safe_blink_toward(my_x, my_y, opp_x, opp_y,
                                               has_blink_u, has_blink_d, has_blink_l, has_blink_r);
            if (blink > 0) return blink;
        }

        // MP 부족하면 REST
        if (my_mp < 2 && dist >= 4) return CMD_REST;

        // 대각선 추격
        return get_diagonal_move_toward(my_x, my_y, opp_x, opp_y);
    }

    /* =====================================================================================
     * PHASE 7: 측면 공격 포지션 (Flanking)
     * ===================================================================================== */
    // 가까이 있지만 직선이 아닐 때 -> 측면 위치 확보
    if (dist <= 2 && my_y != opp_y && my_x != opp_x) {
        int flank = get_flanking_position(my_x, my_y, opp_x, opp_y);
        if (flank > 0) return flank;
    }

    /* =====================================================================================
     * PHASE 8: 기본 추격 (대각선 이동)
     * ===================================================================================== */
    return get_diagonal_move_toward(my_x, my_y, opp_x, opp_y);
}

/* =================================================================================================
 * [PART 9] SYSTEM ENTRY
 * ================================================================================================= */

void student2_ai_entry(void) {
    load_csv_data();

    MY_KEY = register_player_ai("3_Team", my_ai);

    char ans[256];

    // CMD_POISON
    solve_poison(ans);
    attempt_skill_unlock(MY_KEY, CMD_POISON, ans);
    if (is_skill_unlocked(MY_KEY, CMD_POISON))
        printf("3_Team : CMD_POISON 해금 완료\n");
    else
        printf("3_Team : CMD_POISON 해금 실패 ㅜㅜ\n");

    // CMD_STRIKE
    solve_strike(ans);
    attempt_skill_unlock(MY_KEY, CMD_STRIKE, ans);
    if (is_skill_unlocked(MY_KEY, CMD_STRIKE))
        printf("3_Team : CMD_STRIKE 해금 완료\n");
    else
        printf("3_Team : CMD_STRIKE 해금 실패 ㅜㅜ\n");

    // CMD_BLINK 4종
    solve_blink(ans);
    attempt_skill_unlock(MY_KEY, CMD_BLINK_UP, ans);
    attempt_skill_unlock(MY_KEY, CMD_BLINK_DOWN, ans);
    attempt_skill_unlock(MY_KEY, CMD_BLINK_LEFT, ans);
    attempt_skill_unlock(MY_KEY, CMD_BLINK_RIGHT, ans);
    if (is_skill_unlocked(MY_KEY, CMD_BLINK_UP))
        printf("3_Team : CMD_BLINK 4종 해금 완료\n");
    else
        printf("3_Team : CMD_BLINK 4종 해금 실패 ㅜㅜ\n");

    // CMD_HEAL_ALL
    solve_heal_all(ans);
    attempt_skill_unlock(MY_KEY, CMD_HEAL_ALL, ans);
    if (is_skill_unlocked(MY_KEY, CMD_HEAL_ALL))
        printf("3_Team : CMD_HEAL_ALL 해금 완료\n");
    else
        printf("3_Team : CMD_HEAL_ALL 해금 실패 ㅜㅜ\n");

    // CMD_RANGE_ATTACK
    solve_range(ans);
    attempt_skill_unlock(MY_KEY, CMD_RANGE_ATTACK, ans);
    if (is_skill_unlocked(MY_KEY, CMD_RANGE_ATTACK))
        printf("3_Team : CMD_RANGE_ATTACK 해금 완료\n");
    else
        printf("3_Team : CMD_RANGE_ATTACK 해금 실패 ㅜㅜ\n");

    // CMD_BLESS
    solve_self_destruct(ans);
    attempt_skill_unlock(MY_KEY, CMD_BLESS, ans);
    if (is_skill_unlocked(MY_KEY, CMD_BLESS))
        printf("3_Team : CMD_BLESS 해금 완료\n");
    else
        printf("3_Team : CMD_BLESS 해금 실패 ㅜㅜ\n");

    // CMD_H_ATTACK, CMD_V_ATTACK
    solve_hv(ans);
    attempt_skill_unlock(MY_KEY, CMD_H_ATTACK, ans);
    attempt_skill_unlock(MY_KEY, CMD_V_ATTACK, ans);
    if (is_skill_unlocked(MY_KEY, CMD_H_ATTACK))
        printf("3_Team : CMD_H_ATTACK,CMD_V_ATTACK 해금 완료\n");
    else
        printf("3_Team : CMD_H_ATTACK,CMD_V_ATTACK 해금 실패 ㅜㅜ\n");

    // CMD_SECRETE
    solve_secret(ans);
    attempt_skill_unlock(MY_KEY, CMD_SECRETE, ans);
    if (is_skill_unlocked(MY_KEY, CMD_SECRETE)) {
        printf("3_Team : CMD_SECRETE 해금 완료\n");
        set_custom_secrete_message(MY_KEY, "Aggressive Hunter Active - Diagonal Strike Ready!");
    }
    else
        printf("3_Team : CMD_SECRETE 해금 실패 ㅜㅜ\n");

    // 상대 기록 초기화
    opp_history.prev_x = -1;
    opp_history.prev_y = -1;
    opp_history.prev_hp = -1;
    opp_history.prev_mp = -1;
    opp_history.move_dx = 0;
    opp_history.move_dy = 0;
    opp_history.attack_pattern = 0;
    opp_history.consecutive_approach = 0;
    opp_history.consecutive_retreat = 0;
    opp_history.turn_count = 0;

    printf("3_Team : 강화된 공격형 AI 초기화 완료. 아무키나 누르시오.\n");
    getchar();
}
