#include <iostream>
#include <alsa/asoundlib.h>
#include "args.h"

using namespace args;
using namespace std;

static double min_dest = 90;
static string src_ascii_id = R"(name="Speaker Playback Volume")";
static string src_hw = "hw:1";
static std::string dest_ascii_id = R"(iface=CARD,name="Speaker Digital Gain")";
static std::string dest_hw = "hw:1";

int volume_monitor (snd_hctl_elem_t *elem_src, unsigned int _) {
    int err;
    snd_ctl_elem_value_t *value_src;
    snd_ctl_elem_value_alloca(&value_src);
    snd_ctl_elem_info_t *src_info;
    snd_ctl_elem_info_alloca(&src_info);
    snd_ctl_elem_info_t *dest_info;
    snd_ctl_elem_info_alloca(&dest_info);
    auto *elem_dest = (snd_hctl_elem_t *)snd_hctl_elem_get_callback_private(elem_src);
    snd_ctl_elem_value_t *value_dest;
    snd_ctl_elem_value_alloca(&value_dest);

    if ((err = snd_hctl_elem_read(elem_src,value_src))<0) {
        fprintf(stderr, "Error reading src elem on event: %s\n", snd_strerror(err));
        return err;
    };
    long left = snd_ctl_elem_value_get_integer(value_src,0);
    long right = snd_ctl_elem_value_get_integer(value_src,1);

    if ((err = snd_hctl_elem_info(elem_src,src_info))<0) {
        fprintf(stderr, "Error getting src elem info on event: %s\n", snd_strerror(err));
        return err;
    }
    if ((err = snd_hctl_elem_info(elem_dest,dest_info))<0) {
        fprintf(stderr, "Error getting dest elem info on event: %s\n", snd_strerror(err));
        return err;
    }
    auto max_src = snd_ctl_elem_info_get_max(src_info);
    auto max_dest = snd_ctl_elem_info_get_max(dest_info);
    auto src = (double)std::max(left,right);
    long bass;

    if (left+right ==0) {
        bass = 0;
    } else {
        bass = long((double)(max_dest-min_dest)/(double)max_src*src+min_dest);
    }
    fprintf(stdout, "Event read L %ld,R %ld (max %ld) -> bass %ld (max %ld)\n", left,right,max_src,bass,max_dest);

    snd_ctl_elem_value_set_integer(value_dest,0,bass);
    if ((err = snd_hctl_elem_write(elem_dest,value_dest))<0) {
        fprintf(stderr, "Error writing dest elem on event: %s\n", snd_strerror(err));
        return err;
    }

    return 0;
}

int main(int argc, char **argv) {
    setlinebuf(stdout);
    ArgParser parser(R"(
Copy the changes in value of one ALSA control to another.

Usage:
  alsa-controller [flags]

Example:
  alsa-controller -s name="Speaker Playback Volume" -d iface=CARD,name="Speaker Digital Gain" -m 90

Flags:
  -s, --src string          Id of the source control. It will be monitors and its changes will trigger a change in the destination control.
                            (default: name="Speaker Playback Volume")

  -d, --dest string         Id of the destination control. It will be updated each time the source is updated.
                            (default: iface=CARD,name="Speaker Digital Gain")

  -m, --min_dest double     This will be used as a starting value for the destination control when the source is at 1.
                            If the source value is zero, the destination will always be zero too.
                            (default: 90)

  --src_hw string           Id of the source device.
                            (default: hw:1)

  --src_hw string           Id of the destination device.
                            (default: the --src_hw value)
)");
    parser.option("src s",R"(name="Speaker Playback Volume")");
    parser.option("dest d",R"(iface=CARD,name="Speaker Digital Gain")");
    parser.option("min_dest m","90");
    parser.option("src_hw","hw:1");
    parser.option("dest_hw","");
    parser.parse(argc,argv);

    src_ascii_id = parser.value("src");
    src_hw = parser.value("src_hw");
    dest_ascii_id = parser.value("dest");
    if (parser.found("dest_hw"))
        dest_hw = parser.value("dest_hw");
    else
        dest_hw = parser.value("src_hw");
    min_dest = stod(parser.value("min_dest"));

    int err;
    snd_hctl_t *hctl_src,*hctl_dest;
    if ((err = snd_hctl_open(&hctl_src, src_hw.c_str(), 0))<0) {
        fprintf(stderr, "Src card open error: %s\n", snd_strerror(err));
        return err;
    }
    if ((err = snd_hctl_open(&hctl_dest, dest_hw.c_str(), 0))<0) {
        fprintf(stderr, "Dest card open error: %s\n", snd_strerror(err));
        return err;
    }

    if ((err = snd_hctl_load(hctl_src))<0) {
        fprintf(stderr, "High-level src ctl load error: %s\n", snd_strerror(err));
        return err;
    }
    if ((err = snd_hctl_load(hctl_dest))<0) {
        fprintf(stderr, "High-level dest ctl load error: %s\n", snd_strerror(err));
        return err;
    }

    snd_ctl_elem_id_t *id_src;
    snd_ctl_elem_id_t *id_dest;
    snd_ctl_elem_id_alloca(&id_src);
    snd_ctl_elem_id_alloca(&id_dest);
    if ((err = snd_ctl_ascii_elem_id_parse(id_src,src_ascii_id.c_str()))<0) {
        fprintf(stderr, "Unable to find src elem with ascii id %s.\n",src_ascii_id.c_str());
        return err;
    }
    if ((err = snd_ctl_ascii_elem_id_parse(id_dest,dest_ascii_id.c_str()))<0) {
        fprintf(stderr, "Unable to find dest elem with id %s.\n",dest_ascii_id.c_str());
        return err;
    }

    snd_hctl_elem_t *elem_src = snd_hctl_find_elem(hctl_src, id_src);
    if (!elem_src) {
        fprintf(stderr, "Unable to find src elem.\n");
        return err;
    }
    snd_hctl_elem_t *elem_dest = snd_hctl_find_elem(hctl_dest, id_dest);
    if (!elem_dest) {
        fprintf(stderr, "Unable to find dest elem.\n");
        return err;
    }
    snd_hctl_elem_set_callback_private(elem_src,elem_dest);
    snd_hctl_elem_set_callback(elem_src,volume_monitor);
    if ((err =volume_monitor(elem_src,0))<0) {
        fprintf(stderr, "Initial event error: %s\n", snd_strerror(err));
        return err;
    }

    while (true) {
        if (snd_hctl_wait(hctl_src,-1)==0) {
            fprintf(stdout, "Timeout occurred while waiting for events\n");
            continue;
        }
        if ((err = snd_hctl_handle_events(hctl_src))<0) {
            fprintf(stderr, "Handling events error: %s\n", snd_strerror(err));
            return err;
        }
    }
}
