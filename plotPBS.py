import matplotlib.pyplot as plt 



# Build the plot
fig, ax = plt.subplots()

schedulingAlgos = ['Relatively high\npriority to IO\nintensive processes', 'Relatively high\npriority to CPU\nintensive processes', 'Same priority to\n all processes']
x_pos = [0, 1, 2]
meanTicksTaken = [3905, 4181, 3215]



ax.bar(x_pos, meanTicksTaken, align='center', alpha=0.5)
ax.set_ylabel('Mean Time Taken (ticks)')
ax.set_xticks(x_pos)
ax.set_xticklabels(schedulingAlgos)
ax.set_title('Comparison of PBS with different methods of assigning priority')
ax.yaxis.grid(True)

# Save the figure and show
# plt.tight_layout()
plt.savefig('pbs_plot.png')
# plt.show()
plt.close(fig)