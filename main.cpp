#include <iostream>
#include <alsa/asoundlib.h>

static double min_dest = 90;
static std::string src_name = "Speaker Playback Volume";
static std::string dest_name = "Speaker Digital Gain";

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
    if (argc>1 && !strcmp(argv[1],"-h")) {
        fprintf(stdout, "Usage:\n");
        fprintf(stdout, "  alsa-controller <src ctrl name> <dest ctrl name> <min dest volume>\n\n");
        fprintf(stdout, "Example:\n");
        fprintf(stdout, R"(  alsa-controller "Speaker Playback Volume" "Speaker Digital Gain" 90)");
        fprintf(stdout, "\n\n");
        return 0;
    }
    if (argc>1) {
        src_name = argv[1];
    }
    if (argc>2) {
        dest_name = argv[2];
    }
    if (argc>3) {
        min_dest = strtod(argv[3], nullptr);
    }


    int err;
    snd_hctl_t *hctl;
    if ((err = snd_hctl_open(&hctl, "hw:1", 0))<0) {
        fprintf(stderr, "Card open error: %s\n", snd_strerror(err));
        return err;
    }
    if ((err = snd_hctl_load(hctl))<0) {
        fprintf(stderr, "High-level ctl load error: %s\n", snd_strerror(err));
        return err;
    }

    snd_ctl_elem_id_t *id_src;
    snd_ctl_elem_id_t *id_dest;
    snd_ctl_elem_id_alloca(&id_src);
    snd_ctl_elem_id_alloca(&id_dest);
    snd_ctl_elem_id_set_interface(id_src, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_interface(id_dest, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id_src, src_name.c_str());
    snd_ctl_elem_id_set_name(id_dest, dest_name.c_str());

    snd_hctl_elem_t *elem_src = snd_hctl_find_elem(hctl, id_src);
    if (!elem_src) {
        fprintf(stderr, "Unable to find src elem.\n");
        return err;
    }
    snd_hctl_elem_t *elem_dest = snd_hctl_find_elem(hctl, id_dest);
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
        if (snd_hctl_wait(hctl,-1)==0) {
            fprintf(stdout, "Timeout occurred while waiting for events\n");
            continue;
        }
        if ((err = snd_hctl_handle_events(hctl))<0) {
            fprintf(stderr, "Handling events error: %s\n", snd_strerror(err));
            return err;
        }
    }
}
