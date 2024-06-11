#include "stdio.h"
#include "../string.c"

static const char *lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent eros ante, sodales ut commodo nec, sagittis vel sem. Aenean vel odio lectus. \
Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Aenean tristique tempus felis sed pellentesque. Donec eget vestibulum augue. Ut ac nisl venenatis, vehicula felis sit amet, \
gravida purus. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nulla consectetur varius maximus. Morbi aliquet gravida semper. Maecenas lobortis ante felis, sit amet luctus \
neque porttitor non. Proin accumsan, lacus in fermentum condimentum, mi purus porta arcu, ac convallis dolor ipsum eget lorem. Phasellus dapibus nec orci et eleifend. Cras dolor purus, ultricies et convallis id, \
bibendum ut felis. Aliquam aliquet libero sem, et vulputate enim sollicitudin eu. Curabitur sodales, nisl ut bibendum rhoncus, mi nisi auctor dolor, eu sollicitudin magna libero vitae tellus. Nam tempor nisl nulla, \
in consectetur libero rhoncus et.";

int main(int argc, char const *argv[])
{
    StringBuilder sb = string_builder_make(8);
    sb_append(&sb, "%s", lorem);

    char *result = sb_to_string(&sb);

    printf("%s", result);
    return 0;
}
