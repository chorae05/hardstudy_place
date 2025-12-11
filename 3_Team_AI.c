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

/* 전역 상태 */
static int g_poisoned = 0;
static int g_opp_poisoned = 0;
static int g_turn = 0;

static int my_ai(const Player* me, const Player* opp) {
    int my_hp = get_player_hp(me);
    int my_mp = get_player_mp(me);
    int my_x = get_player_x(me);
    int my_y = get_player_y(me);
    int opp_hp = get_player_hp(opp);
    int opp_mp = get_player_mp(opp);
    int opp_x = get_player_x(opp);
    int opp_y = get_player_y(opp);
    int opp_last = get_player_last_command(opp);

    int dx = opp_x - my_x;
    int dy = opp_y - my_y;
    int abs_dx = (dx < 0) ? -dx : dx;
    int abs_dy = (dy < 0) ? -dy : dy;
    int dist = abs_dx + abs_dy;
    int same_row = (my_y == opp_y);
    int same_col = (my_x == opp_x);

    int has_strike = is_skill_unlocked(MY_KEY, CMD_STRIKE);
    int has_bu = is_skill_unlocked(MY_KEY, CMD_BLINK_UP);
    int has_bd = is_skill_unlocked(MY_KEY, CMD_BLINK_DOWN);
    int has_bl = is_skill_unlocked(MY_KEY, CMD_BLINK_LEFT);
    int has_br = is_skill_unlocked(MY_KEY, CMD_BLINK_RIGHT);
    int has_heal_all = is_skill_unlocked(MY_KEY, CMD_HEAL_ALL);
    int has_range = is_skill_unlocked(MY_KEY, CMD_RANGE_ATTACK);
    int has_bless = is_skill_unlocked(MY_KEY, CMD_BLESS);
    int has_h = is_skill_unlocked(MY_KEY, CMD_H_ATTACK);
    int has_v = is_skill_unlocked(MY_KEY, CMD_V_ATTACK);
    int has_poison = is_skill_unlocked(MY_KEY, CMD_POISON);

    g_turn++;
    if (opp_last == CMD_POISON) g_poisoned = 2;
    if (g_poisoned > 0) g_poisoned--;
    if (g_opp_poisoned > 0) g_opp_poisoned--;

    if (my_hp <= 0) return CMD_REST;

    // ===== [0] 확정 킬 - 무조건 최우선 =====
    if (opp_hp == 1) {
        if (dist == 1) return CMD_ATTACK;
        if (dist == 2 && has_range && my_mp >= 1) return CMD_RANGE_ATTACK;
        if (same_row && abs_dx >= 2 && has_h && my_mp >= 3) return CMD_H_ATTACK;
        if (same_col && abs_dy >= 2 && has_v && my_mp >= 3) return CMD_V_ATTACK;
    }
    if (opp_hp == 2) {
        if (dist == 1 && has_strike && my_mp >= 2) return CMD_STRIKE;
        if (same_row && abs_dx >= 2 && has_h && my_mp >= 3) return CMD_H_ATTACK;
        if (same_col && abs_dy >= 2 && has_v && my_mp >= 3) return CMD_V_ATTACK;
    }

    // ===== [1] 생존 - HP 1,2 =====
    if (my_hp == 1) {
        if (opp_hp == 1 && dist == 1) return CMD_ATTACK;
        if (opp_hp <= 2 && dist == 1 && has_strike && my_mp >= 2) return CMD_STRIKE;
        if (g_poisoned > 0 && has_bless && my_mp >= 2) return CMD_BLESS;
        if (has_heal_all && my_mp >= 3) return CMD_HEAL_ALL;
        if (my_mp >= 1) return CMD_HEAL;
        return CMD_REST;
    }

    if (my_hp == 2) {
        if (opp_hp == 1 && dist == 1) return CMD_ATTACK;
        if (opp_hp <= 2 && dist == 1 && has_strike && my_mp >= 2) return CMD_STRIKE;
        if (g_poisoned > 0 && has_bless && my_mp >= 2) return CMD_BLESS;
        if (dist >= 2 && my_mp >= 1) return CMD_HEAL;
        if (dist == 1 && opp_hp >= 3) {
            if (has_br && my_x + 2 <= 7) return CMD_BLINK_RIGHT;
            if (has_bl && my_x - 2 >= 1) return CMD_BLINK_LEFT;
            if (has_bd && my_y + 2 <= 7) return CMD_BLINK_DOWN;
            if (has_bu && my_y - 2 >= 1) return CMD_BLINK_UP;
        }
        if (has_heal_all && my_mp >= 3) return CMD_HEAL_ALL;
        if (my_mp >= 1) return CMD_HEAL;
    }

    // ===== [2] 독 해독 =====
    if (g_poisoned > 0 && my_hp <= 3 && has_bless && my_mp >= 2) {
        return CMD_BLESS;
    }

    // ===== [3] 선독(先毒) 전략 - MP 5 되면 독! =====
    if (my_mp >= 5 && has_poison && g_opp_poisoned == 0) {
        g_opp_poisoned = 2;
        return CMD_POISON;
    }

    // ===== [4] 독 걸었으면 거리 유지하며 공격 =====
    if (g_opp_poisoned > 0) {
        // 킬각
        if (opp_hp == 1 && dist == 1) return CMD_ATTACK;
        if (opp_hp == 1 && dist == 2 && has_range && my_mp >= 1) return CMD_RANGE_ATTACK;
        // 거리 2 유지하며 Range 공격
        if (dist == 2 && has_range && my_mp >= 1) return CMD_RANGE_ATTACK;
        // 근접이면 후퇴
        if (dist == 1) {
            if (has_br && my_x + 2 <= 7 && dx <= 0) return CMD_BLINK_RIGHT;
            if (has_bl && my_x - 2 >= 1 && dx >= 0) return CMD_BLINK_LEFT;
            if (has_bd && my_y + 2 <= 7 && dy <= 0) return CMD_BLINK_DOWN;
            if (has_bu && my_y - 2 >= 1 && dy >= 0) return CMD_BLINK_UP;
            // 그냥 공격
            if (has_strike && my_mp >= 2) return CMD_STRIKE;
            return CMD_ATTACK;
        }
        // 거리 3+면 거리 유지
        if (dist >= 3) {
            if (same_row && has_h && my_mp >= 3) return CMD_H_ATTACK;
            if (same_col && has_v && my_mp >= 3) return CMD_V_ATTACK;
            return CMD_REST;
        }
    }

    // ===== [5] 상대 MP 4-5 = 독 위험! 선제 or 도망 =====
    if (opp_mp >= 4 && dist <= 2) {
        // 내가 먼저 독!
        if (my_mp >= 5 && has_poison && g_opp_poisoned == 0) {
            g_opp_poisoned = 2;
            return CMD_POISON;
        }
        // 킬각
        if (opp_hp == 1 && dist == 1) return CMD_ATTACK;
        if (opp_hp <= 2 && dist == 1 && has_strike && my_mp >= 2) return CMD_STRIKE;
        // 도망
        if (dist <= 1) {
            if (has_br && my_x + 2 <= 7) return CMD_BLINK_RIGHT;
            if (has_bl && my_x - 2 >= 1) return CMD_BLINK_LEFT;
            if (has_bd && my_y + 2 <= 7) return CMD_BLINK_DOWN;
            if (has_bu && my_y - 2 >= 1) return CMD_BLINK_UP;
            if (dx <= 0 && my_x < 7) return CMD_RIGHT;
            if (dx >= 0 && my_x > 1) return CMD_LEFT;
            if (dy <= 0 && my_y < 7) return CMD_DOWN;
            if (dy >= 0 && my_y > 1) return CMD_UP;
        }
    }

    // ===== [6] H/V 회피 =====
    if (opp_last == CMD_H_ATTACK || (same_row && opp_mp >= 3 && dist >= 2)) {
        if (my_y > 1) return CMD_UP;
        if (my_y < 7) return CMD_DOWN;
    }
    if (opp_last == CMD_V_ATTACK || (same_col && opp_mp >= 3 && dist >= 2)) {
        if (my_x > 1) return CMD_LEFT;
        if (my_x < 7) return CMD_RIGHT;
    }

    // ===== [7] MP 5 미만이면 REST로 모으기 =====
    if (my_mp < 5 && has_poison) {
        // 상대가 공격해오면 반격
        if (dist == 1) {
            if (has_strike && my_mp >= 2) return CMD_STRIKE;
            return CMD_ATTACK;
        }
        if (dist == 2 && has_range && my_mp >= 1) return CMD_RANGE_ATTACK;
        // 원거리면 REST
        return CMD_REST;
    }

    // ===== [8] 거리별 공격 =====
    if (dist == 1) {
        if (has_strike && my_mp >= 2) return CMD_STRIKE;
        return CMD_ATTACK;
    }

    if (dist == 2) {
        if (same_row && has_h && my_mp >= 3) return CMD_H_ATTACK;
        if (same_col && has_v && my_mp >= 3) return CMD_V_ATTACK;
        if (has_range && my_mp >= 1) return CMD_RANGE_ATTACK;
        // 접근
        if (abs_dx >= abs_dy) {
            if (dx > 0) return CMD_RIGHT;
            if (dx < 0) return CMD_LEFT;
        }
        if (dy > 0) return CMD_DOWN;
        if (dy < 0) return CMD_UP;
    }

    if (dist >= 3) {
        if (same_row && has_h && my_mp >= 3) return CMD_H_ATTACK;
        if (same_col && has_v && my_mp >= 3) return CMD_V_ATTACK;
        // BLINK 접근
        if (my_mp >= 1) {
            if (has_br && dx > 0 && my_x + 2 <= 7) return CMD_BLINK_RIGHT;
            if (has_bl && dx < 0 && my_x - 2 >= 1) return CMD_BLINK_LEFT;
            if (has_bd && dy > 0 && my_y + 2 <= 7) return CMD_BLINK_DOWN;
            if (has_bu && dy < 0 && my_y - 2 >= 1) return CMD_BLINK_UP;
        }
        // 걸어서
        if (abs_dx >= abs_dy) {
            if (dx > 0) return CMD_RIGHT;
            if (dx < 0) return CMD_LEFT;
        }
        if (dy > 0) return CMD_DOWN;
        if (dy < 0) return CMD_UP;
    }

    return CMD_REST;
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
