import matplotlib.pyplot as plt 



# Build the plot
fig, ax = plt.subplots()

schedulingAlgos = ['FCFS', 'MLFQ', 'RR (default)']
x_pos = [0, 1, 2]
meanTicksTaken = [4574, 3395, 3137]



ax.bar(x_pos, meanTicksTaken, align='center', alpha=0.5)
ax.set_ylabel('Mean Time Taken (ticks)')
ax.set_xticks(x_pos)
ax.set_xticklabels(schedulingAlgos)
ax.set_title('Comparison of Different Scheduling Methods')
ax.yaxis.grid(True)

# Save the figure and show
plt.tight_layout()
plt.savefig('bar_plot.png')
# plt.show()
plt.close(fig)